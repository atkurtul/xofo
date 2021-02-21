

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <CommonInterfaces/CommonCameraInterface.h>
#include <btBulletDynamicsCommon.h>

#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btAabbUtil2.h"
#include "core.h"
#include "imgui.h"
#include "pipeline.h"

#include <shape.h>
#include <vulkan/vulkan_core.h>
#include <xofo.h>

using namespace std;
using namespace xofo;

void draw_f32x4(const char* name, f32x4 v) {
  ImGui::Text("%s %5f %5f %5f %5f", name, (f32)v.x, (f32)v.y, (f32)v.z,
              (f32)v.w);
}

struct Xform {
  f32x4x4 xform;
  f32x3 vel;

  Xform(f32x3 pos, f32x3 vel) : xform(translate(f32x4x4(0.1), pos)), vel(vel) {}

  f32x4x4 const& integrate(f32 dt) {
    return xform = translate(xform, vel * dt);
  }
};

struct PhysicsSimulator {
  Box<btDefaultCollisionConfiguration> config;
  Box<btCollisionDispatcher> dispatcher;
  Box<btBroadphaseInterface> cache;
  Box<btSequentialImpulseConstraintSolver> solver;
  Box<btDiscreteDynamicsWorld> world;
  vector<Box<btCollisionShape>> shapes;

  PhysicsSimulator()
      : config(new btDefaultCollisionConfiguration{}),
        dispatcher(new btCollisionDispatcher(config.get())),
        cache(new btDbvtBroadphase),
        world(new btDiscreteDynamicsWorld(dispatcher.get(),
                                          cache.get(),
                                          solver.get(),
                                          config.get())) {
    world->setGravity(btVector3(0, -10, 0));

    {
      btCollisionShape* ground = new btBoxShape(
          btVector3(btScalar(400.), btScalar(1.), btScalar(400.)));

      shapes.push_back(Box<btCollisionShape>(ground));

      btTransform xf;
      xf.setIdentity();
      xf.setOrigin(btVector3(0, -1, 0));

      // using motionstate is optional, it provides interpolation capabilities,
      // and only synchronizes 'active' objects
      btDefaultMotionState* myMotionState = new btDefaultMotionState(xf);
      btRigidBody::btRigidBodyConstructionInfo rbInfo(0, myMotionState, ground,
                                                      {});
      btRigidBody* body = new btRigidBody(rbInfo);
      // body->setFlags(1);
      // add the body to the dynamics world
      world->addRigidBody(body);
    }
  }

  btRigidBody* add_body(f32x3 bbMin,
                        f32x3 bbMax,
                        f32x3 pos,
                        f32x3 initial_velocity) {
    auto half = (bbMax - bbMin) * 0.5f;
    f32 mass = half.x * half.y * half.z;
    bool isDynamic = (mass != 0.f);

    auto shape = new btBoxShape(btVector3(half.x, half.y, half.z));

    shapes.push_back(Box<btBoxShape>(shape));
    btTransform startTransform;
    startTransform.setIdentity();

    btVector3 localInertia(0, 0, 0);
    if (isDynamic)
      shape->calculateLocalInertia(mass, localInertia);
    startTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));

    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        mass, new btDefaultMotionState(startTransform), shape, localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);
    body->setLinearVelocity(
        btVector3(initial_velocity.x, initial_velocity.y, initial_velocity.z));
    world->addRigidBody(body);
    body->getUserIndex();
    return body;
  }

  auto cast_ray_(btVector3 pos, btVector3 ray) {
    btCollisionWorld::AllHitsRayResultCallback cb(pos, ray);
    world->rayTest(pos, ray, cb);
    return cb;
  }

  auto cast_ray(f32x4 pos, f32x4 ray) {
    pos = pos + ray;
    ray = pos + ray * 1000.f;
    return cast_ray_((btVector3&)pos, (btVector3&)ray);
  }

  vector<f32x4x4> get_matricies() {
    auto arr = world->getCollisionWorld()->getCollisionObjectArray();
    vector<f32x4x4> re(arr.size() - 1);
    for (u32 i = 1; i < arr.size(); ++i) {
      arr[i]->getWorldTransform().getOpenGLMatrix(&re[i - 1][0][0]);
    }
    return re;
  }

  void clear() {
    auto arr = world->getCollisionWorld()->getCollisionObjectArray();
    for (u32 i = 1; i < arr.size(); ++i) {
      btRigidBody* body = btRigidBody::upcast(arr[i]);
      if (body && body->getMotionState())
        delete body->getMotionState();

      world->removeCollisionObject(arr[i]);
      delete arr[i];
    }
  }

  void integrate(f32 t) { world->stepSimulation(t); }

  ~PhysicsSimulator() {
    for (int i = world->getNumCollisionObjects() - 1; i >= 0; i--) {
      btCollisionObject* obj = world->getCollisionObjectArray()[i];
      btRigidBody* body = btRigidBody::upcast(obj);
      if (body && body->getMotionState())
        delete body->getMotionState();

      world->removeCollisionObject(obj);
      delete obj;
    }
  }
};

f32x4x4 inv_scale(Bbox box) {
  auto s = 1.f / abs(box.max - box.min);
  f32x4x4 m;
  m[0][0] = s.x;
  m[1][1] = s.y;
  m[2][2] = s.z;
  return m;
}

int main() {
  xofo::init();

  auto pipeline = Pipeline::mk("shaders/shader");

  auto skybox = Pipeline::mk("shaders/skybox", PipelineState{.depth_write = 0});

  CubeMap cube_map(*skybox);

  Model cube("models/cube.gltf", *pipeline);

  Cube box;
  // vector<Xform> xforms;

  PhysicsSimulator sim;

  auto grid_pipe = Pipeline::mk(
      "shaders/grid", PipelineState{.polygon = VK_POLYGON_MODE_LINE,
                                    .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                                    .line_width = 1.2f});

  auto uniform = Buffer::mk(65536, Buffer::Uniform, Buffer::Mapped);

  // Model model0("models/sponza/sponza.gltf", *pipeline);
  // Model model1("models/doom/0.obj", *pipeline);
  // Model model2("models/batman/0.obj", *pipeline);

  xofo::Camera cam(1600, 900);

  xofo::register_recreation_callback(
      [&](auto extent) { cam.set_prj(extent.width, extent.height); });

  f32x4 ax(0, 0, 0, 1);
  f32x3 xxf(0);

  auto grid_buffer = Buffer::mk(65536, Buffer::Vertex, Buffer::Mapped);
  for (u32 i = 0; i <= 2000; ++i) {
    f32x3* verts = (f32x3*)grid_buffer->mapping;
    i32 half = 1000;
    f32 x = (f32)i - half;

    verts[i * 4 + 0] = f32x3(x, 0, half);
    verts[i * 4 + 1] = f32x3(x, 0, -half);
    verts[i * 4 + 2] = f32x3(half, 0, x);
    verts[i * 4 + 3] = f32x3(-half, 0, x);
  }

  f32 speed = 1.f;
  f32x4 force;

  while (xofo::poll()) {
    f64 dt = xofo::dt();
    auto mdelta = mouse_delta() * -0.0012f;

    if (xofo::mbutton(1))
      xofo::hide_mouse(1);
    else {
      xofo::hide_mouse(0);
      mdelta = {};
    }

    f32 w = xofo::get_key(Key::W);
    f32 a = xofo::get_key(Key::A);
    f32 s = xofo::get_key(Key::S);
    f32 d = xofo::get_key(Key::D);
    cam.update(mdelta, s - w, d - a, dt * 5 * speed);

    uniform->copy(cam.pos);

  
    sim.integrate(dt);

    {
      static float timer = 0;
      if ((timer += dt) > 0.1 && get_key(Key::E)) {
        timer = 0;
        sim.add_body(0, 1, cam.pos.xyz + 2.f * cam.mouse_ray.xyz,
                     10 * cam.mouse_ray.xyz);
        // xforms.emplace_back(cam.pos.xyz, cam.mouse_ray.xyz);
        // xforms.emplace_back(cam.pos.xyz, -cam.ori[2].xyz);
      }
      if (get_key(Key::R))
        sim.clear();
    }

    xofo::draw(
        [&](auto) {
          skybox->bind();
          skybox->push(cam.view);
          skybox->push(cam.prj, 64);

          cube_map.draw(*skybox, translate(f32x4x4(1), f32x3(cam.pos.xyz)));

          pipeline->bind();
          pipeline->bind_set([&](auto set) { uniform->bind_to_set(set, 0); },
                             {uniform.get()});

          // model0.draw(*pipeline, scale<f32,4>(0.1));
          // model1.draw(*pipeline, f32x4x4(1));
          // model2.draw(*pipeline, f32x4x4(1));
          auto xforms = sim.get_matricies();
          auto scale = inv_scale(cube.box);
          for (auto& xform : xforms) {
            cube.draw(*pipeline, scale * xform);
          }

          grid_pipe->bind();
          grid_pipe->push(f32x4x4(1), 128);
          grid_buffer->bind_vertex();
          vkCmdDraw(vk, 8004, 1, 0, 0);

          // {
          //   static float timer = 0;
          //   static bool toggler = true;
          //   if ((timer += dt) > 0.2 && get_key(Key::SPACE))
          //     (timer = 0), toggler = !toggler;
          //   if (toggler) {
          //     ((f32x3*)grid_buffer->mapping)[8004] = cam.pos;
          //     ((f32x3*)grid_buffer->mapping)[8005] =
          //         cam.pos.xyz + cam.mouse_ray.xyz * 10.f;
          //   }
          // }

          // box.draw(xforms, *grid_pipe);
        },
        [&]() {
          using namespace ImGui;
          ImGui::BeginMainMenuBar();
          {
            {
              static float timer = 0, dtt = dt;
              if ((timer += dt) > 0.2)
                timer = 0, dtt = 1.f / dt;
              ImGui::Text("%f\n", dtt);
            }

            if (ImGui::BeginMenu("File")) {
              ImGui::EndMenu();
            }
          }

          ImGui::Begin("Dirs");
          draw_f32x4("Mouse ray", cam.mouse_ray);
          draw_f32x4("Forward", cam.ori[2]);
          draw_f32x4("Pos", cam.pos);
          draw_f32x4("Force", force);
          ImGui::End();

          ImGui::EndMainMenuBar();

          for (auto pipe : Pipeline::pipelines) {
            pipe->show();
          }

          // model2.show();
        });
  }
  CHECKRE(vkDeviceWaitIdle(xofo::vk));
}
