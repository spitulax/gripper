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
  version = with lib; elemAt
    (pipe (readFile ../meson.build) [
      (splitString "\n")
      (filter (hasPrefix "  version : "))
      head
      (splitString " : ")
      last
      (splitString "'")
    ]) 1;
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

  meta = with lib; {
    platforms = platforms.linux;
    description = "Simple and easy to use screenshot utility for Wayland";
    mainProgram = [ "gripper" ];
    homepage = "https://github.com/spitulax/gripper";
    license = licenses.mit;
    maintainers = with maintainers; [ spitulax ];
  };
}
