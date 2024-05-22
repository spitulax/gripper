{ stdenv
, lib
, grim
, slurp
, wl-clipboard
, libnotify
, makeWrapper
, debug ? false
}:
let
  version = with lib; removePrefix "v" (elemAt
    (pipe (readFile ../src/main.c) [
      (splitString "\n")
      (filter (hasPrefix "#define PROG_VERSION"))
      head
      (splitString " ")
      last
      (splitString "\"")
    ]) 1);
in
stdenv.mkDerivation rec {
  pname = "waysnip";
  inherit version;
  src = lib.cleanSource ./..;

  nativeBuildInputs = [
    makeWrapper
  ];

  postInstall = ''
    wrapProgram $out/bin/${pname} \
      --suffix PATH : ${lib.makeBinPath [
        grim
        slurp
        wl-clipboard
        libnotify
      ]}
  '';

  makeFlags = [ "PREFIX=$(out)" ] ++ lib.optionals (!debug) [ "RELEASE=1" ];
}
