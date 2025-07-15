{
  description = "C/C++ Dev Environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
    python.url = "github:nix-community/pynixpkgs";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };

        pythonEnv = pkgs.python3.withPackages (ps: with ps; [
          setuptools-scm
          pallets-sphinx-themes
          sphinxcontrib-plantuml
        ]);

      in {
        devShell = pkgs.mkShell {
          name = "cpp-dev-env";

          nativeBuildInputs = [
            pkgs.meson
            pkgs.ninja
            pkgs.gcc15
            pkgs.clang_20
            pkgs.cppcheck
            pkgs.make
            pkgs.include-what-you-use
            pkgs.cspell
            pkgs.clang-tools_20
            pkgs.binutils
            pkgs.gdb
            pkgs.ccache
            pkgs.doxygen
            pkgs.python3Packages.sphinx
            pkgs.python3Packages.breathe
            pkgs.yamllint
            pkgs.plantuml
            pythonEnv
          ];

          # Optional: to ensure correct versions (informational only — strict versioning in Nix is complex)
          shellHook = ''
            echo "Welcome to the C/C++ development environment"
            echo "Meson version: $(meson --version)"
            echo "GCC version: $(gcc --version | head -n1)"
            echo "Clang version: $(clang --version | head -n1)"
          '';
        };
      }
    );
}
