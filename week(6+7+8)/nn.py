
import torch
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader
import pandas as pd
import chess
from tqdm import tqdm

device = torch.device("cuda" if torch.cuda.is_available() else "cpu") # faster training on gpu

piece_to_base = {
    (chess.PAWN, True):0,(chess.KNIGHT, True):64,(chess.BISHOP, True):128, # 768 features 64*12 
    (chess.ROOK, True):192,(chess.QUEEN, True):256,(chess.KING, True):320,
    (chess.PAWN, False):384,(chess.KNIGHT, False):448,(chess.BISHOP, False):512,
    (chess.ROOK, False):576,(chess.QUEEN, False):640,(chess.KING, False):704
}

def fen_to_vector(fen):
    b=chess.Board(fen)
    v=[0.0]*773
    for s in chess.SQUARES:
        p=b.piece_at(s)
        if p:
            v[piece_to_base[(p.piece_type,p.color)]+s]=1.0
    v[768]=float(b.turn)
    v[769]=float(b.has_kingside_castling_rights(chess.WHITE)) # castle features
    v[770]=float(b.has_queenside_castling_rights(chess.WHITE))
    v[771]=float(b.has_kingside_castling_rights(chess.BLACK))
    v[772]=float(b.has_queenside_castling_rights(chess.BLACK))
    return v

class ChessDataset(Dataset):
    def __init__(self,csv):
        df=pd.read_csv(csv)
        self.X=[]
        self.Y=[]
        for row in tqdm(df.itertuples(index=False), total=len(df), desc="Preparing dataset"):
            self.X.append(fen_to_vector(row.FEN))
            self.Y.append(max(min(float(row.Evaluation), 1500), -1500) / 1500.0)
        self.X=torch.tensor(self.X,dtype=torch.float32)
        self.Y=torch.tensor(self.Y,dtype=torch.float32)
    def __len__(self):
        return len(self.Y)
    def __getitem__(self,idx):
        return self.X[idx],self.Y[idx]

class ChessNet(nn.Module):
    def __init__(self):
        super().__init__()
        self.net=nn.Sequential(nn.Linear(773,512),nn.ReLU(),nn.Linear(512,256),nn.ReLU(),nn.Linear(256,64),nn.ReLU(),nn.Linear(64,1)) # hidden layers 773 to 512 to 256 to 64 to 1
    def forward(self,x):
        return self.net(x)

dataset=ChessDataset("chess_dataset.csv") 
loader = DataLoader(dataset,batch_size=512,shuffle=True,num_workers=0,pin_memory=True)
model=ChessNet().to(device)
criterion=nn.SmoothL1Loss()
optimizer=torch.optim.AdamW(model.parameters(),lr=1e-3, weight_decay=1e-4)
best=float("inf")

for epoch in range(10):
    model.train()
    total=0.0
    bar=tqdm(loader,desc=f"Epoch {epoch+1}")
    for X,Y in bar:
        X=X.to(device,non_blocking=True)
        Y=Y.view(-1,1).to(device,non_blocking=True)
        optimizer.zero_grad()
        pred=model(X)
        loss=criterion(pred,Y)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(),1.0)
        optimizer.step()
        total+=loss.item()
        bar.set_postfix(loss=f"{loss.item():.5f}")
    avg=total/len(loader)
    if avg<best:
        best=avg
        torch.save(model.state_dict(),"best_model.pth")

model.load_state_dict(torch.load("best_model.pth",map_location=device))
model.eval()

def evaluate_fen(fen):
    with torch.no_grad():
        x=torch.tensor([fen_to_vector(fen)],dtype=torch.float32,device=device)
        return model(x).item()*1500
