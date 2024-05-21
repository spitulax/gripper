{ self, lib, inputs }: {
  default = final: prev: rec {
    wayshot = final.callPackage ./default.nix { };
    wayshot-debug = wayshot.override { debug = true; };
  };
}
