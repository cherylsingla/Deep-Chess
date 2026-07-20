import torch
import torch.nn as nn

class ChessNet(nn.Module):
    def __init__(self):
        super().__init__()
        self.net=nn.Sequential(nn.Linear(773,512),nn.ReLU(),nn.Linear(512,256),nn.ReLU(),nn.Linear(256,64),nn.ReLU(),nn.Linear(64,1))
    def forward(self,x):
        return self.net(x)

model=ChessNet()
model.load_state_dict(torch.load("best_model.pth",map_location="cpu"))
model.eval()
weights=[]
biases=[]

for layer in model.net:
    if isinstance(layer,nn.Linear):
        weights.append(layer.weight.detach().numpy())
        biases.append(layer.bias.detach().numpy())

def write_matrix(f,name,arr):
    rows=len(arr)
    cols=len(arr[0])
    f.write(f"constexpr float {name}[{rows}][{cols}]={{\n")

    for row in arr:
        f.write("    {")
        f.write(",".join(f"{float(x):.8f}f" for x in row))
        f.write("},\n")

    f.write("};\n\n")

def write_vector(f,name,arr):
    f.write(f"constexpr float {name}[{len(arr)}]={{")
    f.write(",".join(f"{float(x):.8f}f" for x in arr))
    f.write("};\n\n")

with open("nn_weights.h","w") as f:
    f.write("#pragma once\n\n")

    for i,(w,b) in enumerate(zip(weights,biases),1):
        write_matrix(f,f"FC{i}_WEIGHT",w)
        write_vector(f,f"FC{i}_BIAS",b)
