import base64
import struct
import math

fileName = "dude-bones"
  
boneMeshFile = open(fileName + ".bonemesh", "rb")

print(boneMeshFile.read())

elements = struct.unpack("8f4B4f", boneMeshFile.read())

for e in elements:
    print(e)
