{ pkgs ? import <nixpkgs> {}
, buildType ? "Release"
}:

assert builtins.elem buildType ["Release" "Debug"];

pkgs.stdenv.mkDerivation {
  pname = "VLSIProject";
  version = "0.1";

  src = ./.;

  nativeBuildInputs = with pkgs; [ cmake ];

  buildInputs = [ ];

  cmakeFlags = [ "-DCMAKE_BUILD_TYPE=${buildType}" ];

  meta = with pkgs.lib; {
    description = "Buffer insertion alghoritm implementation";
    platforms = platforms.all;
  };
}
