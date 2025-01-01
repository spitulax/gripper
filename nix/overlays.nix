{ self, lib, inputs }: {
  default = final: prev: {
    gripper = final.callPackage ./default.nix { };
    gripper-debug = final.callPackage ./default.nix { debug = true; };
  };
}
