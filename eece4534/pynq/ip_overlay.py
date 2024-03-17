from pynq import Overlay

print("Generating fcl_accel overlay...")

ol = Overlay('/home/xilinx/pynq/overlays/fcl_accel/fcl_accel.bit')
ol.download()

print("Done.")
