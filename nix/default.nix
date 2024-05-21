{ stdenv
, lib
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
stdenv.mkDerivation {
  pname = "wayshot";
  inherit version;
  src = lib.cleanSource ./..;

  buildInputs = [

  ];

  makeFlags = [ "PREFIX=$(out)" ] ++ lib.optionals (!debug) [ "RELEASE=1" ];
}
