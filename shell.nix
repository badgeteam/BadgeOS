let
  nixpkgs-rev = "d8fd1dd81c18a31844d08d63207f6c4a9e2c51ba";  # release-24.05 on 2024-08-14
  nixpkgs-hash = "sha256:002nvkg7ak7a39963fw4vad2pm14c3rimj00z115pzrlflx6h4nl";
  nixpkgs-src = (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/${nixpkgs-rev}.tar.gz";
    sha256 = nixpkgs-hash;
  });

  esp-dev-rev = "86a2bbe01fe0258887de7396af2a5eb0e37ac3be"; # main on 2024-08-04
  esp-dev-hash = "sha256:1sj2dz0a0rsmpm5igyxiy8sj8j8scsz1b6hcs0z4spwp83bc6lvv";
  esp-dev-src = (builtins.fetchTarball {
    url = "https://github.com/mirrexagon/nixpkgs-esp-dev/archive/${esp-dev-rev}.tar.gz";
    sha256 = esp-dev-hash;
  });
in

{ pkgs ? import nixpkgs-src {
  overlays = [
    (import "${esp-dev-src}/overlay.nix")
  ];
}
}:

with pkgs;
let
  riscv32-unknown-linux-gnu = stdenv.mkDerivation rec {
    name = "riscv32-unknown-linux-gnu";
    version = "13.2.0";

    release = "2024.03.01";
    ubuntu_version = "22.04";

    src = (builtins.fetchTarball "https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/${release}/riscv32-glibc-ubuntu-${ubuntu_version}-gcc-nightly-${release}-nightly.tar.gz");

    buildInputs = [
      autoPatchelfHook

      expat
      glib
      gmp
      libmpc
      lzma
      mpfr
      ncurses
      python310
      zlib
      zstd
    ];

    installPhase = ''
      runHook preInstall

      cp -r . $out

      runHook postInstall
    '';
  };

  # esptool-git = esptool.overrideAttrs(finalAttrs: previousAttrs: rec{
  #   version = "4d0c7d9";
  #   src = fetchFromGitHub {
  #     owner = "espressif";
  #     repo = "esptool";
  #     rev = "${version}";
  #     hash = "sha256-O9rqCAMK+1JMMCJFl5GggEJqQ0LCP436wf2MCEo2gE0=";
  #   };
  # });
in
mkShell {
  buildInputs = [
    clang-tools_18
    cmake
    esp-idf-esp32c6
    jq
    picocom
    riscv32-unknown-linux-gnu
  ];

  CLANG_FORMAT = "clang-format";
  CLANG_TIDY = "clang-tidy";
}
