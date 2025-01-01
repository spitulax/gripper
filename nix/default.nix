{ stdenv
, lib
, grim
, slurp
, wl-clipboard
, libnotify
, makeWrapper
, meson
, ninja
, pkg-config
, jq

, debug ? false
}:
let
  version = lib.trim (lib.readFile ../VERSION);
in
stdenv.mkDerivation rec {
  pname = "gripper";
  inherit version;
  src = lib.cleanSource ./..;

  nativeBuildInputs = [
    meson
    ninja
    pkg-config
    makeWrapper
  ];

  mesonBuildType = if debug then "debug" else "release";

  postInstall = ''
    wrapProgram $out/bin/${pname} \
      --suffix PATH : ${lib.makeBinPath [
        grim
        slurp
        wl-clipboard
        libnotify
        jq
      ]}
  '';

  meta = {
    platforms = lib.platforms.linux;
    description = "Simple and easy to use screenshot utility for Wayland";
    mainProgram = "gripper";
    homepage = "https://github.com/spitulax/gripper";
    license = lib.licenses.mit;
  };
}
