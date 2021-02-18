#!/usr/bin/env perl
use feature qw(say);


$out_name = "@ARGV[0].spv";

`glslc @ARGV[0] -c -o $out_name`;

$shader = `spirv-reflect $out_name`;

unlink $out_name;

# $shader = join("\n", grep(!/^\s*$/, split("\n", $shader)));

say $shader;

$shader =~ /Input variables: (\d+)/;
$in_vars = $1 * 7;
$shader =~ /Input variables: $1\s*\n((.*\n){$in_vars})/;
$inputs = $1;
$inputs =~ s/ +/ /g;
@inputs = split /\d+:/, $inputs;

for($i = 1; $i < @inputs; ++$i) {
  @inputs[$i] =~ /location : (\d+)/;
  $loc = $1;
  @inputs[$i] =~ /type : (\w+)/;

  if ($loc >= 10) {
    $instance_max = $loc > $instance_max ? $loc : $instance_max;
    $instance_stage_inputs{$loc} = $1;
    continue; 
  }

  $max = $loc > $max ? $loc : $max;
  $stage_inputs{$loc} = $1;
}

for($i = 0; $i <= $max; ++$i) {

  if (!exists($stage_inputs{$i})) {
    exit -1;
  }

  say "$i := <$stage_inputs{$i}> [Vertex]";
}
