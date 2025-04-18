{
  description = "Buffer insertion algorithm implementation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        
        buildWith = buildType: pkgs.stdenv.mkDerivation {
          pname = "VLSIProject";
          version = "0.1";
          
          src = ./.;
          
          nativeBuildInputs = with pkgs; [
            cmake
            nlohmann_json
          ];
          
          buildInputs = [ ];
          
          cmakeFlags = [ "-DCMAKE_BUILD_TYPE=${buildType}" ];
          
          meta = with pkgs.lib; {
            description = "Buffer insertion algorithm implementation";
            platforms = platforms.all;
          };
        };
      in
      {
        packages = {
          default = buildWith "Release";
          
          release = buildWith "Release";
          debug = buildWith "Debug";
        };
        
        devShells.default = pkgs.mkShell {
          name = "vlsi-dev-shell";
          
          packages = with pkgs; [
            cmake
            nlohmann_json
            ccache
            gdb
            valgrind
            clang-tools
            gtest
          ];
          
        };
      }
    );
}