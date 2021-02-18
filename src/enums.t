#!/usr/bin/env perl

use feature qw(say);

if ("$ENV{VULKAN_SDK}") {
  $vkpath = "$ENV{VULKAN_SDK}/include/vulkan/vulkan_core.h";
} else {
  $vkpath = "/usr/include/vulkan/vulkan_core.h";
}

open $fh, "<", $vkpath or die $!;


while($line = <$fh>) {
  $file .= $line;
}

open $cpp, ">", "src/maps.cpp";
open $header, ">", "include/maps.h";

say $cpp "#include <maps.h>";
say $header "#include <core.h>";

while ($file =~ /typedef\s+enum\s+(\w+)\s+\{((.|\n)*?)\}/g) {
  $enum = $1;
  $elems = $2;
  $elems =~ s/\s+=.*\n/,/g;
  $elems =~ s/\s+/ /g;
  $enum =~ s/Vk//g;
  $enum = "const std::vector<std::pair<std::string, Vk${enum}>> ${enum}_map";

  say $header "extern $enum;";
  say $cpp "$enum = {";
    for $ele (split(",", $elems)) {
      if (!($ele =~ /MAX_ENUM/g)) {
        $str = substr $ele, 4;
        $str =~ s/_/ /g;
        say $cpp "  {\"$str\", $ele},";
      }
    }
  say $cpp "};";
}

close $fh;
close $cpp;
close $header;