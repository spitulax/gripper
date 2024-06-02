{ self, lib, inputs }: {
  default = final: prev: rec {
    gripper = final.callPackage ./default.nix { };
    gripper-debug = gripper.override { debug = true; };
  };
}
