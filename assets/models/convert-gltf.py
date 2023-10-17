import json
import base64
import struct
import math

fileName = "gubbe"
  
gltfFile = open(fileName + ".gltf")
#gltfFile = open('minimalfile.gltf')
  
data = json.load(gltfFile)

#
# CREATE BONE MESH FILE
#

n_positions = data["accessors"][0]["count"]
n_normals = data["accessors"][1]["count"]
n_texCoords = data["accessors"][2]["count"]
n_joints = data["accessors"][3]["count"]
n_weights = data["accessors"][4]["count"]
n_indices = data["accessors"][5]["count"]

bufferBytes = base64.b64decode(data["buffers"][0]["uri"].split(",")[1])

positionBytes = bufferBytes[data["bufferViews"][0]["byteOffset"]:(data["bufferViews"][0]["byteOffset"] + data["bufferViews"][0]["byteLength"])]
normalBytes   = bufferBytes[data["bufferViews"][1]["byteOffset"]:(data["bufferViews"][1]["byteOffset"] + data["bufferViews"][1]["byteLength"])]
texCoordBytes = bufferBytes[data["bufferViews"][2]["byteOffset"]:(data["bufferViews"][2]["byteOffset"] + data["bufferViews"][2]["byteLength"])]
jointBytes    = bufferBytes[data["bufferViews"][3]["byteOffset"]:(data["bufferViews"][3]["byteOffset"] + data["bufferViews"][3]["byteLength"])]
weightBytes   = bufferBytes[data["bufferViews"][4]["byteOffset"]:(data["bufferViews"][4]["byteOffset"] + data["bufferViews"][4]["byteLength"])]
indexBytes    = bufferBytes[data["bufferViews"][5]["byteOffset"]:(data["bufferViews"][5]["byteOffset"] + data["bufferViews"][5]["byteLength"])]

positions = struct.unpack("<" + str(n_positions * 3) + "f", positionBytes)
normals = struct.unpack(str(n_normals * 3) + "f", normalBytes)
texCoords = struct.unpack(str(n_texCoords * 2) + "f", texCoordBytes)
joints = struct.unpack(str(n_joints * 4) + "B", jointBytes)
weights = struct.unpack(str(n_weights * 4) + "f", weightBytes)
indices = struct.unpack(str(n_indices) + "H", indexBytes)

print(min(indices))
print(max(indices))

print(n_positions)
print(n_indices)

outputData = b""

n = 0
for i in indices:

    #print(n, triangleN)

    #if positions[i * 3 + 1] < 0.5:
        #print(joints[i * 4 + 0], joints[i * 4 + 1], joints[i * 4 + 2], joints[i * 4 + 3])
        #print(joints[i * 4])
    #if joints[i * 4 + 0] == 0 and joints[i * 4 + 1] == 0 and joints[i * 4 + 2] == 0 and joints[i * 4 + 3] == 0:
        #print("zero indices")

    #print(n)
    #print(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2])
    #print(normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2])
    #print(texCoords[i * 2 + 0], texCoords[i * 2 + 1])
    #print(joints[i * 4 + 0], joints[i * 4 + 1], joints[i * 4 + 2], joints[i * 4 + 3])
    #print(weights[i * 4 + 0], weights[i * 4 + 1], weights[i * 4 + 2], weights[i * 4 + 3])
    #n += 1

    outputData += struct.pack(
        "8f4B4f",
        positions[i * 3 + 0],
        positions[i * 3 + 1],
        positions[i * 3 + 2],
        normals[i * 3 + 0],
        normals[i * 3 + 1],
        normals[i * 3 + 2],
        texCoords[i * 2 + 0],
        texCoords[i * 2 + 1],
        #1,
        #1,
        #1,
        #1,
        joints[i * 4 + 0],
        joints[i * 4 + 1],
        joints[i * 4 + 2],
        joints[i * 4 + 3],
        #max(joints[i * 4 + 0] - 1, 0),
        #max(joints[i * 4 + 1] - 1, 0),
        #max(joints[i * 4 + 2] - 1, 0),
        #max(joints[i * 4 + 3] - 1, 0),
        weights[i * 4 + 0],
        weights[i * 4 + 1],
        weights[i * 4 + 2],
        weights[i * 4 + 3],
    )

    n += 1

n_triangles = n_indices / 3

outputFile = open(fileName + ".bonemesh", "wb")

outputFile.write(outputData)

outputFile.close()

#
# CREATE BONES FILE
#

tmp_n_nodes = 0
tmp_names = []
tmp_rotations = []
tmp_scales = []
tmp_translations = []
tmp_parents = []

for nodeData in data["nodes"]:

    tmp_names.append(nodeData["name"])

    if "rotation" in nodeData:
        tmp_rotations.append(nodeData["rotation"])
    else:
        tmp_rotations.append([0.0, 0.0, 0.0, 1.0])

    if "scale" in nodeData:
        tmp_scales.append(nodeData["scale"])
    else:
        tmp_scales.append([1.0, 1.0, 1.0])

    if "translation" in nodeData:
        tmp_translations.append(nodeData["translation"])
    else:
        tmp_translations.append([0.0, 0.0, 0.0])

    tmp_parents.append(-1)

    tmp_n_nodes += 1

for i in range(0, tmp_n_nodes):
    if "children" in data["nodes"][i]:
        for j in data["nodes"][i]["children"]:
            tmp_parents[j] = i
            #print(j)

n_nodes = 0
names = []
rotations = []
scales = []
translations = []
parents = []

#sort names and stuff
for index in data["skins"][0]["joints"]:
    #print(index)
    names.append(tmp_names[index])
    rotations.append(tmp_rotations[index])
    scales.append(tmp_scales[index])
    translations.append(tmp_translations[index])

    found = False
    for parentIndex in range(0, len(data["skins"][0]["joints"])):
        if(data["skins"][0]["joints"][parentIndex] == tmp_parents[index]):
            parents.append(parentIndex)
            #print("found")
            found = True
            break

    if not found:
        parents.append(-1)
#parents.append(data["skins"][0]["joints"][tmp_parents[index]])
    n_nodes += 1

outputFile = open(fileName + ".bones", "w")

for i in range(0, n_nodes):
    outputFile.write(":BONE\n")

    outputFile.write(names[i] + "\n")

    outputFile.write(str(parents[i]) + "\n")

    for x in rotations[i]:
        outputFile.write("{:.6f} ".format(x))
    outputFile.write("\n")

    for x in scales[i]:
        outputFile.write("{:.6f} ".format(x))
    outputFile.write("\n")

    for x in translations[i]:
        outputFile.write("{:.6f} ".format(x))
    outputFile.write("\n")

    #print(names[i])
    #print("me: " + str(i))
    #print("parent: " + str(parents[i]))
    #print(rotations[i])
    #print(scales[i])
    #print(translations[i])

outputFile.close()
