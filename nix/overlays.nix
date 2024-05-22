{ self, lib, inputs }: {
  default = final: prev: rec {
    waysnip = final.callPackage ./default.nix { };
    waysnip-debug = waysnip.override { debug = true; };
  };
}
