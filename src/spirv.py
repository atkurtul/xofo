#!/usr/bin/env python

from subprocess import run, PIPE
import sys
import re

def check_exit_code(proc):
  if (proc.returncode != 0):
    sys.stderr.write(proc.stderr.decode("utf-8"))
    exit(-1)

out_name = sys.argv[1] + ".spv";

glslc = run(["glslc", sys.argv[1], "-c" ,"-o", out_name], stderr=PIPE)

check_exit_code(glslc)

spirv = run(["spirv-reflect", out_name], stderr=PIPE, stdout=PIPE)

check_exit_code(spirv)

reflect = spirv.stdout.decode("utf-8").replace("\r\n", "\n")
#print(reflect)
reflect = list(filter(lambda s: s != "", reflect.split("\n")))

inputs=[]
descriptors=[]

re_input=re.compile("^\s*Input variables\s*:\s*(\d+)\s*$")
re_desc=re.compile("^\s*Descriptor bindings\s*:\s*(\d+)\s*$")

for i, line in enumerate(reflect):

  if not inputs:
    if match:=re_input.search(line):
      input_count=int(match.group(1))
      for j in range(input_count):
        grp="".join(reflect[i+1 + 7*j: i+8 + 7*j])
        loc=int(re.search("location\s*:\s*(\d+)",grp).group(1))
        ty=str(re.search("type\s*:\s*(\w+)",grp).group(1))
        inputs.append((loc, ty))
  
  if not descriptors:
    if match:=re_desc.search(line):
      desc_count=int(match.group(1))
      for j, line in enumerate(reflect[i+1:]):
        if line.strip().startswith("Binding"):
          grp="".join(reflect[i+j+2:i+j+8])
          setn=int(re.search("set\s*:\s*(\d+)",grp).group(1))
          bind=int(re.search("binding\s*:\s*(\d+)",grp).group(1))
          count=int(re.search("count\s*:\s*(\d+)",grp).group(1))
          accessed=bool(re.search("accessed\s*:\s*(\w+)",grp).group(1))
          ty=str(re.search("type\s*:\s*(\w+)",grp).group(1)).replace("VK_DESCRIPTOR_TYPE_", "")
          descriptors.append((setn, bind, ty, count, accessed))

