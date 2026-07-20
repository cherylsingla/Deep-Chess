#include "chess.hpp"
#include <bits/stdc++.h>
#include<chrono>
using namespace std;
using namespace chess;

chrono::steady_clock::time_point start,now;
long long elapsed,time_allow;
long long nodes = 0;
Move killer[64][2];
int historyTable[2][64][64];
Move prev_best = Move::NO_MOVE;
bool stop_search=false;
const int pawnTable[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
    50,  50,  50,  50,  50,  50,  50,  50,
    10,  10,  20,  30,  30,  20,  10,  10,
     5,   5,  10,  25,  25,  10,   5,   5,
     0,   0,   0,  20,  20,   0,   0,   0,
     5,  -5, -10,   0,   0, -10,  -5,   5,
     5,  10,  10, -20, -20,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0
};
const int knightTable[64] = {
-50,-40,-30,-30,-30,-30,-40,-50,
-40,-20,  0,  0,  0,  0,-20,-40,
-30,  0, 10, 15, 15, 10,  0,-30,
-30,  5, 15, 20, 20, 15,  5,-30,
-30,  0, 15, 20, 20, 15,  0,-30,
-30,  5, 10, 15, 15, 10,  5,-30,
-40,-20,  0,  5,  5,  0,-20,-40,
-50,-40,-30,-30,-30,-30,-40,-50
};
const int bishopTable[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};
const int rookTable[64] = {
     0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};
const int queenTable[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};
const int kingTable[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};
const int kingEndgameTable[64] = {
   -50,-30,-30,-30,-30,-30,-30,-50,
   -30,-10,  0,  0,  0,  0,-10,-30,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,-10,  0,  0,  0,  0,-10,-30,
   -50,-30,-30,-30,-30,-30,-30,-50
};

struct TTEntry{
    uint64_t key;
    int depth;
    int score;
    Move bestMove;
    char flag;
};

constexpr char EXACT = 0;
constexpr char LOWER = 1;
constexpr char UPPER = 2;
constexpr int TT_SIZE = 1 << 20;
vector<TTEntry> TT(TT_SIZE);

int game_phase(Board &board){
    int phase = 24;
    for(int i = 0; i < 64; i++){
        Piece piece = board.at(Square(i));
        if(piece == Piece::NONE)
            continue;
        if(piece.type() == PieceType::QUEEN)
            phase -= 4;
        else if(piece.type() == PieceType::ROOK)
            phase -= 2;
        else if(piece.type() == PieceType::BISHOP)
            phase -= 1;
        else if(piece.type() == PieceType::KNIGHT)
            phase -= 1;
    }
    if(phase < 0) phase = 0;
    return phase;
}

int pieceValue(Piece piece){
        if(piece.type() == PieceType::PAWN) return 100;
        else if(piece.type() == PieceType::KNIGHT) return 305;
        else if(piece.type() == PieceType::BISHOP) return 333;
        else if(piece.type() == PieceType::ROOK) return 563;
        else if(piece.type() == PieceType::QUEEN) return 950;
        return 0;
    }

int leastValuableAttacker(Board &board, Bitboard attackers){
    const int INF =100000000;
    int best = INF;
    while(!attackers.empty()){
        int sq = attackers.pop();
        Piece p = board.at(Square(sq));
        best = min(best, pieceValue(p));
    }
    if(best==INF) return 0;
    return best;
}

int SEE(Board &board, Move move){
    if(!board.isCapture(move)) return 0;
    Piece attacker = board.at(move.from());
    Piece victim = board.at(move.to());
    int score = pieceValue(victim)-pieceValue(attacker);
    board.makeMove(move);
    Bitboard recaptures =attacks::attackers(board,board.sideToMove(),move.to());
    if(!recaptures.empty()) score -= leastValuableAttacker(board,recaptures);
    board.unmakeMove(move);
    return score;
}

int moveScore(Board& board,Move move,int depth,Move ttMove){
    if(move==ttMove)
        return 100000;
    if(move.typeOf()==Move::PROMOTION){
        int score=30000;
        if(board.isCapture(move)){
            Piece victim=board.at(move.to());
            Piece attacker=board.at(move.from());
            score+=20000+10*pieceValue(victim)-pieceValue(attacker);
        }
        return score;
    }
    if(board.isCapture(move)){
        int see = SEE(board, move);
        if(see >= 0)
            return 30000 + see;   
        else
            return 15000 + see;   
    }
    if(move==killer[depth][0])
        return 9000;
    if(move==killer[depth][1])
        return 8000;
    int side;
    if(board.sideToMove() == Color::WHITE) side = 0;
    else side = 1;
    return historyTable[side][move.from().index()][move.to().index()];
}

int captureScore(Board& board,Move move){
    int score=0;
    if(board.isCapture(move))
        score+= 10000+10*SEE(board,move);
    if(move.typeOf()==Move::PROMOTION)
        score+=8000;
    return score;
}

int evaluateMaterialPST(Board &board){
    int phase=game_phase(board);
    int score=0,whiteBishops=0,blackBishops=0;
    for(int i=0;i<64;i++){
        Square sq(i);
        Piece piece=board.at(sq);
        if(piece==Piece::NONE)
            continue;
        int value=0;
        int index=sq.relative_square(piece.color()).index();
        if(piece.type()==PieceType::PAWN)
            value=100+pawnTable[index];
        else if(piece.type()==PieceType::BISHOP){
            value=333+bishopTable[index];
            if(piece.color()==Color::WHITE)
                whiteBishops++;
            else
                blackBishops++;
        }
        else if(piece.type()==PieceType::KNIGHT)
            value=305+knightTable[index];
        else if(piece.type()==PieceType::ROOK)
            value=563+rookTable[index];
        else if(piece.type()==PieceType::QUEEN)
            value=950+queenTable[index];
        else if(piece.type()==PieceType::KING){
            int opening=kingTable[index];
            int endgame=kingEndgameTable[index];
            value=((24-phase)*opening+phase*endgame)/24;
        }
        if(piece.color()==Color::WHITE)
            score+=value;
        else
            score-=value;
    }
    if(whiteBishops>=2)
        score+=30;
    if(blackBishops>=2)
        score-=30;
    return score;
}

int evaluatePawns(Board &board){
    int score=0;
    int phase=game_phase(board);
    vector<Square>white_pawns,black_pawns;
    int white_file_count[8]={0};
    int black_file_count[8]={0};
    for(int i=0;i<64;i++){
        Square sq(i);
        Piece piece=board.at(sq);
        if(piece==Piece::NONE||piece.type()!=PieceType::PAWN)  continue;
        if(piece.color()==Color::WHITE){
            white_pawns.push_back(sq);
            white_file_count[sq.file()]++;
        }
        else{
            black_pawns.push_back(sq);
            black_file_count[sq.file()]++;
        }
    }
    const int passed_pawn_bonus[8]={0,5,10,20,35,60,100,0};
    for(Square sq:black_pawns){
        int file=sq.file();
        int rank=sq.rank();
        bool passed=true;
        for(Square enemy:white_pawns){
            int enemy_file=enemy.file();
            int enemy_rank=enemy.rank();
            if(abs(enemy_file-file) <= 1 && enemy_rank < rank){
                passed=false;
                break;
            }
        }
        if(passed){
            int bonus = passed_pawn_bonus[7-rank]; // passed pawns wala stuff
            bonus = bonus*(24+phase)/24;
            score-=bonus;
        }
        if(black_file_count[file]>1)  score+=12; // double pawns ke liye (bad)
        bool isolated=true; // isolated pawns bhi bad
        if(file > 0 && black_file_count[file-1] > 0 ) isolated = false;
        if(file < 7 && black_file_count[file+1] > 0) isolated = false;
        if(isolated) score+=9;
        bool connected = false;
        for(Square p:black_pawns){
            if(p==sq)  continue;
            if(abs(p.file()-file)==1 && abs(p.rank()-rank) <= 1){ // connected pawns gud 
                connected=true;
                break;
            }
        }
        if(connected) score-=10;
    }
    for(Square sq:white_pawns){
        int file=sq.file();
        int rank=sq.rank();
        bool passed=true;
        for(Square enemy:black_pawns){
            int enemy_file=enemy.file();
            int enemy_rank=enemy.rank();
            if(abs(enemy_file-file) <=1 && enemy_rank > rank){
                passed=false;
                break;
            }
        }
        if(passed){
            int bonus = passed_pawn_bonus[rank];
            bonus = bonus*(24+phase)/24;
            score+=bonus;
        }
        if(white_file_count[file]>1)  score-=12;
        bool isolated=true;
        if(file > 0 && white_file_count[file-1] > 0)  isolated=false;
        if(file < 7 && white_file_count[file+1] > 0)  isolated=false;
        if(isolated)  score-=9;
        bool connected=false;
        for(Square p:white_pawns){
            if(p==sq)
                continue;
            if(abs(p.file()-file)==1 && abs(p.rank()-rank)<=1){
                connected=true;
                break;
            }
        }
        if(connected)  score+=10;
    }
    return score;
}

int evaluateKingSafety(Board &board){
    int score=0;
    int phase=game_phase(board);
    Square whiteKing,blackKing;
    for(int i=0;i<64;i++){
        Square sq(i);
        Piece piece=board.at(sq);
        if(piece==Piece::NONE)
            continue;
        if(piece.type()==PieceType::KING){
            if(piece.color()==Color::WHITE)
                whiteKing=sq;
            else
                blackKing=sq;
        }
    }
    int wf=(int)whiteKing.file();
    int wr=(int)whiteKing.rank();
    for(int i=-1;i<=1;i++){
        int nf=wf+i;
        int nr=wr+1;
        if(nf<0||nf>7||nr>7)
            continue;
        Piece p=board.at(Square(File(nf),Rank(nr)));
        if(p.type()==PieceType::PAWN && p.color()==Color::WHITE)  // king surrounded by pawn gud for safety
            score+=8; 
        else
            score-=8;
    }
    if(phase<12){
        if((wf==6 && wr==0)||(wf==2 && wr==0)) //opening time king on castling positions preferred
            score+=12;
    }
    for(int f=max(0,wf-1);f<=min(7,wf+1);f++){
        bool friendlyPawn=false;
        for(int r=0;r<8;r++){
            Piece p=board.at(Square(File(f),Rank(r)));
            if(p.type()==PieceType::PAWN && p.color()==Color::WHITE){
                friendlyPawn=true;
                break;
            }
        }
        if(!friendlyPawn) //no pawn in surrounding columsn around king
            score-=12;
    }
    int bf=(int)blackKing.file();
    int br=(int)blackKing.rank();
    for(int i=-1;i<=1;i++){
        int nf=bf+i;
        int nr=br-1;
        if(nf<0||nf>7||nr<0)
            continue;
        Piece p=board.at(Square(File(nf),Rank(nr)));
        if(p.type()==PieceType::PAWN && p.color()==Color::BLACK)
            score-=8;
        else
            score+=8;
    }
    if(phase<12){
        if((bf==6 && br==7)||(bf==2 && br==7))
            score-=12;
    }
    for(int f=max(0,bf-1);f<=min(7,bf+1);f++){
        bool friendlyPawn=false;
        for(int r=0;r<8;r++){
            Piece p=board.at(Square(File(f),Rank(r)));
            if(p.type()==PieceType::PAWN && p.color()==Color::BLACK){
                friendlyPawn=true;
                break;
            }
        }
        if(!friendlyPawn)
            score+=12;
    }
    return score;
}

int evaluateKingActivity(Board &board){
    if(game_phase(board)<18)
        return 0;
    int score=0;
    Square whiteKing,blackKing;
    for(int i=0;i<64;i++){
        Square sq(i);
        Piece piece=board.at(sq);
        if(piece==Piece::NONE) continue;
        if(piece.type()==PieceType::KING){
            if(piece.color()==Color::WHITE)
                whiteKing=sq;
            else
                blackKing=sq;
        }
    }
    int wf=whiteKing.file();
    int wr=whiteKing.rank();
    int bf=blackKing.file();
    int br=blackKing.rank();
    int whiteDist=min(abs(wf-3)+abs(wr-3),abs(wf-4)+abs(wr-4));
    int blackDist=min(abs(bf-3)+abs(br-3),abs(bf-4)+abs(br-4));
    score+=(6-whiteDist)*4;
    score-=(6-blackDist)*4;
    for(int i=0;i<64;i++){
        Square sq(i);
        Piece piece=board.at(sq);
        if(piece==Piece::NONE) continue;
        if(piece.type()!=PieceType::PAWN) continue;
        if(piece.color()==Color::BLACK){
            int dist=abs(wf-sq.file())+abs(wr-sq.rank());
            score+=(14-dist);
        }
        else{
            int dist=abs(bf-sq.file())+abs(br-sq.rank());
            score-=(14-dist);
        }
    }
    return score;
}

int evaluateRooks(Board &board){
    int score = 0;
    bool whitePawn = false,blackPawn = false;
    for(int i = 0; i< 64;i++){
        Piece p = board.at(Square(i));
        if(p == Piece::NONE || p.type() != PieceType::PAWN)
            continue;
        if(p.color() == Color::WHITE)
            whitePawn = true;
        else
            blackPawn = true;
    }
    for(int i=0;i<64;i++){
        Square sq(i);
        Piece piece = board.at(sq);
        if(piece == Piece::NONE || piece.type() != PieceType::ROOK)
            continue;
        int file = sq.file();
        int rank = sq.rank();
        bool own_pawn = false, enemy_pawn = false;
        for(int r = 0; r < 8; r++){
            Piece p = board.at(Square(File(file), Rank(r)));
            if(p == Piece::NONE || p.type() != PieceType::PAWN) 
                continue;
            if(p.color() == piece.color())
                own_pawn = true;
            else
                enemy_pawn = true;
        }
        if(!own_pawn && !enemy_pawn){ // pawns not blocking activity
            if(piece.color() == Color::WHITE)
                score += 20;
            else
                score -= 20;
        }
        else if(!own_pawn){
            if(piece.color() == Color::WHITE) // opp pawn 
                score += 10;
            else
                score -= 10;
        }
        if(piece.color() == Color::WHITE && rank == 6 && blackPawn) 
            score += 20;
        if(piece.color() == Color::BLACK && rank == 1 && whitePawn)
            score -= 20;
    }
    return score;
}
int evaluateDevelopment(Board &board){
    int score=0;
    if(game_phase(board)>=12) return 0;
    Square whiteKing = board.kingSq(Color::WHITE);
    Square blackKing = board.kingSq(Color::BLACK);
    if(game_phase(board)<12){
        if(whiteKing == Square(File::FILE_G, Rank::RANK_1) || whiteKing == Square(File::FILE_C, Rank::RANK_1)) score += 15;
        if(blackKing == Square(File::FILE_G, Rank::RANK_8) || blackKing == Square(File::FILE_C, Rank::RANK_8)) score -= 15;
    }
    if(board.at(Square(File::FILE_B, Rank::RANK_1)).type()==PieceType::KNIGHT)  score-=8;
    if(board.at(Square(File::FILE_G, Rank::RANK_1)).type()==PieceType::KNIGHT) score-=8;
    if(board.at(Square(File::FILE_C, Rank::RANK_1)).type()==PieceType::BISHOP) score-=8;
    if(board.at(Square(File::FILE_F, Rank::RANK_1)).type()==PieceType::BISHOP) score-=8;

    if(board.at(Square(File::FILE_B, Rank::RANK_8)).type()==PieceType::KNIGHT) score+=8;
    if(board.at(Square(File::FILE_G, Rank::RANK_8)).type()==PieceType::KNIGHT) score+=8;
    if(board.at(Square(File::FILE_C, Rank::RANK_8)).type()==PieceType::BISHOP) score+=8;
    if(board.at(Square(File::FILE_F, Rank::RANK_8)).type()==PieceType::BISHOP) score+=8;
    return score;
}

int evaluateMobility(Board &board){
    int score=0;
    Bitboard occupied=board.occ();
    for(int i=0;i<64;i++){
        Square sq(i);
        Piece piece=board.at(sq);
        if(piece==Piece::NONE) continue;
        Bitboard attacks;
        if(piece.type()==PieceType::KNIGHT)  attacks=attacks::knight(sq);
        else if(piece.type()==PieceType::BISHOP) attacks=attacks::bishop(sq,occupied);
        else if(piece.type()==PieceType::ROOK)  attacks=attacks::rook(sq,occupied);
        else if(piece.type()==PieceType::QUEEN) attacks=attacks::queen(sq,occupied);
        else continue;
        //remove our own pieces
        attacks &= ~board.us(piece.color()); // only count pos in attack in  which humare piece na ho
        int moves=attacks.count();
        int bonus=0;
        if(piece.type()==PieceType::KNIGHT) bonus=(moves-4)*4;
        else if(piece.type()==PieceType::BISHOP) bonus=(moves-7)*3;
        else if(piece.type()==PieceType::ROOK) bonus=(moves-7)*2;
        else if(piece.type()==PieceType::QUEEN) bonus=(moves-14);
        if(piece.color()==Color::WHITE) score+=bonus;
        else score-=bonus;
    }
    return score;
}

int evaluateKnightOutposts(Board &board){
    if(game_phase(board)>18)
        return 0;
    int score = 0;
    for(int i=0;i<64;i++){
        Square sq(i);
        Piece piece=board.at(sq);
        int rank = sq.rank();
        int file = sq.file();
        Bitboard pawns,support;
        if(piece.type()!=PieceType::KNIGHT) continue;
        if(piece.color()==Color::WHITE){
            if(rank<3 || rank>5) continue;}
        else{
            if(rank<2 || rank>4) continue;}
        
        if(piece.color()==Color::WHITE) pawns = board.pieces(PieceType::PAWN, Color::WHITE);
        else pawns = board.pieces(PieceType::PAWN, Color::BLACK);

        if(piece.color()==Color::WHITE)
            support = attacks::pawn(Color::BLACK, sq);
        else
            support = attacks::pawn(Color::WHITE, sq);
        bool supported = (support & pawns).count() > 0;
        bool enemyPawnCanAttack = false;
        for(int j=0;j<64;j++){
            Piece p = board.at(Square(j));
            if(p.type()!=PieceType::PAWN) continue;
            if(p.color()==piece.color()) continue;
            if(attacks::pawn(p.color(),Square(j)) & Bitboard::fromSquare(sq)){
                    enemyPawnCanAttack=true;
                    break;
            }
        }
        if(supported && !enemyPawnCanAttack){
            int bonus = 15;
            if(file>=2 && file<=5) bonus += 10;
            if(piece.color()==Color::WHITE) score += bonus;
            else score -= bonus;
        }
    }
    return score;
}

int evaluateCenter(Board &board){
    int score=0;
    Bitboard occupied=board.occ();
    Square center[4]={
        Square(File::FILE_D,Rank::RANK_4),
        Square(File::FILE_E,Rank::RANK_4),
        Square(File::FILE_D,Rank::RANK_5),
        Square(File::FILE_E,Rank::RANK_5)
    };
    for(int i=0;i<64;i++){
        Square sq(i);
        Piece piece=board.at(sq);
        if(piece==Piece::NONE) continue;
        int bonus=0;
        for(int j=0;j<4;j++){
            if(sq!=center[j]) continue;
            if(piece.type()==PieceType::PAWN) bonus+=8;
            else if(piece.type()==PieceType::KNIGHT) bonus+=12;
            else if(piece.type()==PieceType::BISHOP) bonus+=10;
            else if(piece.type()==PieceType::ROOK) bonus+=6;
            else if(piece.type()==PieceType::QUEEN) bonus+=4;
        }
        Bitboard attack;
        if(piece.type()==PieceType::KNIGHT) 
            attack=attacks::knight(sq);
        else if(piece.type()==PieceType::BISHOP)
            attack=attacks::bishop(sq,occupied);
        else if(piece.type()==PieceType::ROOK)
            attack=attacks::rook(sq,occupied);
        else if(piece.type()==PieceType::QUEEN)
            attack=attacks::queen(sq,occupied);
        else if(piece.type()==PieceType::KING)
            attack=attacks::king(sq);
        else if(piece.type()==PieceType::PAWN)
            attack=attacks::pawn(piece.color(),sq);
        else
            continue;

        for(int j=0;j<4;j++)
            if(attack & Bitboard::fromSquare(center[j])) bonus+=2;
        if(piece.color()==Color::WHITE)
            score+=bonus;
        else
            score-=bonus;
    }
    return score;
}

int evaluateThreats(Board &board){
    int score = 0;
    Bitboard occupied = board.occ();
    for(int i=0;i<64;i++){
        Square sq(i);
        Piece piece = board.at(sq);
        if(piece==Piece::NONE || piece.type()==PieceType::KING) continue;
        Bitboard attack;
        if(piece.type()==PieceType::PAWN)
            attack = attacks::pawn(piece.color(),sq);
        else if(piece.type()==PieceType::KNIGHT)
            attack = attacks::knight(sq);
        else if(piece.type()==PieceType::BISHOP)
            attack = attacks::bishop(sq,occupied);
        else if(piece.type()==PieceType::ROOK)
            attack = attacks::rook(sq,occupied);
        else
            attack = attacks::queen(sq,occupied);

        for(int j=0;j<64;j++){
            if(!attack.check(j))  continue;
            Piece target = board.at(Square(j));
            if(target==Piece::NONE)  continue;
            if(target.color()==piece.color())  continue;
            int bonus = 0;

            // Pawn is attacker baaki victims
            if(piece.type()==PieceType::PAWN){
                if(target.type()==PieceType::KNIGHT) bonus = 11;
                else if(target.type()==PieceType::BISHOP) bonus = 10;
                else if(target.type()==PieceType::ROOK) bonus = 16;
                else if(target.type()==PieceType::QUEEN) bonus = 35;
            }

            else if(piece.type()==PieceType::KNIGHT){
                if(target.type()==PieceType::BISHOP) bonus = 6;
                else if(target.type()==PieceType::ROOK) bonus = 12;
                else if(target.type()==PieceType::QUEEN) bonus = 20;
            }

            else if(piece.type()==PieceType::BISHOP){
                if(target.type()==PieceType::KNIGHT) bonus = 5;
                else if(target.type()==PieceType::ROOK) bonus = 12;
                else if(target.type()==PieceType::QUEEN) bonus = 20;
            }

            else if(piece.type()==PieceType::ROOK){
                if(target.type()==PieceType::KNIGHT) bonus = 8;
                else if(target.type()==PieceType::BISHOP) bonus = 8;
                else if(target.type()==PieceType::QUEEN) bonus = 18;
            }

            else if(piece.type()==PieceType::QUEEN){
                if(target.type()==PieceType::ROOK) bonus = 6;
                else if(target.type()==PieceType::BISHOP) bonus = 4;
                else if(target.type()==PieceType::KNIGHT) bonus = 4;
            }
            Bitboard defenders = attacks::attackers(board, piece.color(), sq);
            bool defended = defenders.count()>1;
            if(!defended)
                bonus /= 2;
            if(piece.color()==Color::WHITE)
                score += bonus;
            else
                score -= bonus;
        }
    }

    return score;
}

int evaluatePushKingEdge(Board &board){
    if(game_phase(board) < 18)
        return 0;
    int material = evaluateMaterialPST(board);
    if(abs(material) < 700)  return 0;

    Square whiteKing, blackKing;
    for(int i=0;i<64;i++){
        Piece piece = board.at(Square(i));
        if(piece == Piece::NONE) continue;
        if(piece.type() == PieceType::KING){
            if(piece.color() == Color::WHITE)
                whiteKing = Square(i);
            else
                blackKing = Square(i);
        }
    }
    int score = 0;
    int kingDist = abs(whiteKing.file() - blackKing.file()) + abs(whiteKing.rank() - blackKing.rank());
    int blackEdge = min(min((int)blackKing.file(), 7 - (int)blackKing.file()),min((int)blackKing.rank(), 7 - (int)blackKing.rank()));
    int whiteEdge = min(min((int)whiteKing.file(), 7 - (int)whiteKing.file()),min((int)whiteKing.rank(), 7 - (int)whiteKing.rank()));
    if(material > 700){
        score += (14 - kingDist) * 5;
        score += (3 - blackEdge) * 20;
    }
    else if(material < -700){
        score -= (14 - kingDist) * 5;
        score -= (3 - whiteEdge) * 20;
    }
    return score;
}

int evaluate(Board &board){
    int score = 0;
    score += evaluateMaterialPST(board);
    score += evaluatePawns(board);
    score += evaluateRooks(board);
    if(game_phase(board)<18)
        score+=evaluateKingSafety(board);
    score += evaluateMobility(board);
    score+=evaluateDevelopment(board);
    score+=evaluateKnightOutposts(board);
    score+=evaluateCenter(board);
    score+=evaluateKingActivity(board);
    score+=evaluateThreats(board);
    score+=evaluatePushKingEdge(board);
    return score;
}

int quiescence(Board &board,int alpha,int beta,int ply){
    nodes++;
    if((nodes & 2047)==0){ // check time after every 2048 nodes hi wrna bahut expensive time wise
        now=chrono::steady_clock::now();
        elapsed=chrono::duration_cast<chrono::milliseconds>(now-start).count();
        if(elapsed>=time_allow){
            stop_search=true;
            int eval=evaluate(board);
            if (board.sideToMove()==Color::WHITE) return eval;
            else return -eval;
        }
    }
    if(board.isRepetition()) return 0; //3 fold rule
    if(board.isHalfMoveDraw()) return 0; //50 move
    if(board.isInsufficientMaterial()) return 0;
    bool inCheck = board.inCheck();
    Movelist legalMoves;
    movegen::legalmoves<movegen::MoveGenType::ALL>(legalMoves,board);
    if(legalMoves.empty()){
        if(inCheck) return -1000000+ply;
        else return 0;
    }
    int static_eval = evaluate(board);
    if(board.sideToMove()==Color::BLACK) static_eval=-static_eval;
    if(!inCheck){
        if(static_eval>=beta) return beta;
        alpha=max(alpha,static_eval);
    }
    Movelist moves;
    if(inCheck)
        moves = legalMoves; 
    else
        movegen::legalmoves<movegen::MoveGenType::CAPTURE>(moves,board);
    sort(moves.begin(),moves.end(),[&](Move a,Move b){return captureScore(board,a) > captureScore(board,b);});
    for(auto move:moves){
        if(board.isCapture(move) && SEE(board,move)<-50) continue;
        board.makeMove(move);
        int score=-quiescence(board,-beta,-alpha,ply+1); // negative obv hai kyuki black ka acha is white ka bura
        board.unmakeMove(move);
        if(stop_search) return score;
        if(score>=beta) return beta;
        alpha=max(alpha,score);
    }
    return alpha;
}

vector<Move> orderedMoves(Board &board, int depth){
    Movelist moves_help;
    movegen::legalmoves<movegen::MoveGenType::ALL>(moves_help, board);
    uint64_t key = board.hash(); // hash code for board position (chess hpp ka function directly used)
    Move ttMove = Move::NO_MOVE;
    int index = key % TT_SIZE;
    if(TT[index].key == key)
        ttMove = TT[index].bestMove;
    vector<pair<int, Move>> scored;
    for(auto move : moves_help)
        scored.push_back({moveScore(board, move, depth, ttMove), move});
    sort(scored.begin(), scored.end(), [](const auto &a, const auto &b){return a.first > b.first;}); // sort on basis of first value in pair i.e score
    vector<Move> movelist;
    for(const auto &p : scored)
        movelist.push_back(p.second);
    return movelist;
}

bool hasNonPawnMaterial(Board &board, Color c){ //other than pawn something left(obv king left)
    for(int i=0;i<64;i++){
        Piece p = board.at(Square(i));
        if(p==Piece::NONE || p.color()!=c) continue;
        if(p.type()!=PieceType::PAWN && p.type()!=PieceType::KING) return true;
    }
    return false;
}

const int INF = 100000000; // overflow prevent
int alpha_beta(Board &board, int depth, int alpha, int beta,int ply){
    nodes++;
    if((nodes & 2047) == 0){
        now = chrono::steady_clock::now();
        elapsed = chrono::duration_cast<chrono::milliseconds>(now - start).count();
        if(elapsed >= time_allow){
            stop_search = true;
            int eval = evaluate(board);
            if(board.sideToMove() == Color::WHITE) return eval;
            else return -eval;
        }
    }
    if(stop_search){
        int eval = evaluate(board);
        if(board.sideToMove() == Color::WHITE) return eval;
            else return -eval;
    }
    if(depth == 0){
        return quiescence(board, alpha, beta,ply);
    }
    uint64_t key = board.hash();
    TTEntry &entry = TT[key % TT_SIZE];
    constexpr int MATE = 1000000;
    if(entry.key == key && entry.depth >= depth){
        int ttScore = entry.score;
        if(ttScore > MATE-500) ttScore -= ply;
        else if(ttScore < 500-MATE) ttScore += ply;
        if(entry.flag == EXACT) return ttScore;
        if(entry.flag == LOWER) alpha = max(alpha,ttScore);
        else if(entry.flag == UPPER) beta = min(beta,ttScore);
        if(alpha >= beta) return ttScore;
    }
    if(board.isRepetition()){
        int matEval = evaluateMaterialPST(board);
        int eval = (board.sideToMove() == Color::WHITE) ? matEval : -matEval;
        if(eval>300) return -10;  
        if(eval< -300) return 10; 
        return 0;
    }
    if(board.isHalfMoveDraw()) return 0; //50 move
    if(board.isInsufficientMaterial()) return 0;
 /* if(depth >= 3 && !board.inCheck() && hasNonPawnMaterial(board, board.sideToMove())){ //doing nothing is bad than doing move except when we have only pawns
        board.makeNullMove();
        int score = -alpha_beta(board, depth - 3, -beta, -beta + 1,ply+1);
        board.unmakeNullMove();
        if(stop_search) return score;
        if(score >= beta) return beta;
    } */
    int alphaOrig = alpha;
    int betaOrig = beta;
    vector<Move> movelist = orderedMoves(board, depth);
    if(movelist.empty()){
        if(board.inCheck())
            return -1000000 + ply;
        return 0;
    }
    int bestScore = -INF;
    Move bestMove = Move::NO_MOVE;
    board.makeMove(movelist[0]);
    int score = -alpha_beta(board, depth - 1, -beta, -alpha,ply+1); //pehlli move
    board.unmakeMove(movelist[0]);
    alpha=max(alpha,score);
    bestScore=score;
    bestMove=movelist[0];
    if(stop_search)  return score;
    if(alpha >= beta){
        int helper = bestScore;
        if(helper > MATE-500) helper += ply;
        else if(helper < 500-MATE) helper -= ply;
        entry.key = key;
        entry.depth = depth;
        entry.score = helper;
        entry.bestMove = bestMove;
        entry.flag = LOWER;
        return bestScore;  
    }

    for(long long unsigned int i=1;i<movelist.size();i++){ // PVS
        board.makeMove(movelist[i]);
        bool inCheck = board.inCheck();
        if(i>=4 && depth>=3 && !inCheck &&  !board.isCapture(movelist[i]) && movelist[i].typeOf()!=Move::PROMOTION) score=-alpha_beta(board, depth-2, -alpha-1, -alpha,ply+1); //LMR
        else score=-alpha_beta(board, depth - 1, -alpha-1, -alpha,ply+1);
        if(score>alpha && score<beta) score= -alpha_beta(board, depth - 1, -beta, -alpha,ply+1);
        board.unmakeMove(movelist[i]);
        if(score > bestScore){
            bestScore = score;
            bestMove = movelist[i];
        }
        if(score > alpha)
            alpha = score;
        if(alpha >= beta){
            if(!board.isCapture(movelist[i])){
                int side = (board.sideToMove() == Color::WHITE) ? 0 : 1;
                historyTable[side][movelist[i].from().index()][movelist[i].to().index()] += depth * depth;
                killer[depth][1] = killer[depth][0];
                killer[depth][0] = movelist[i];
            }
            break;
        }
    }
    if(stop_search)
        return bestScore;
    if(entry.key != key || depth >= entry.depth){
        int helper = bestScore;
        if(helper > MATE-500) helper += ply;
        else if(helper < -MATE+500) helper -= ply;
        entry.key = key;
        entry.depth = depth;
        entry.score = helper;
        entry.bestMove = bestMove;

        if(bestScore <= alphaOrig)
            entry.flag = UPPER;
        else if(bestScore >= betaOrig)
            entry.flag = LOWER;
        else
            entry.flag = EXACT;
    }
    return bestScore;
}

Move Bestmove(Board &board, int depth){
    for(int c = 0; c < 2; c++)
        for(int from = 0; from < 64; from++)
            for(int to = 0; to < 64; to++)
                historyTable[c][from][to] /= 2;
    vector<Move> movelist = orderedMoves(board, depth);
    if(movelist.empty())
        return Move::NO_MOVE;
    int bestScore = -INF;
    Move bestMove = Move::NO_MOVE;
    for(auto move : movelist){
        if(stop_search)
            break;
        board.makeMove(move);
        int score = -alpha_beta(board, depth-1, -INF, INF, 1);
        board.unmakeMove(move);
        cout<<uci::moveToUci(move)<<" "<<score<<endl;
        if(score > bestScore){
            bestScore = score;
            bestMove = move;
            prev_best = move;
        }
    }
    return bestMove;
}

int main(){
    string command;
    Board board;
    while(getline(cin,command)){
        if(command=="uci"){
            cout<<"id name CherylBot"<<endl;
            cout<<"id author Cheryl"<<endl;
            cout<<"uciok"<<endl;
        }
        else if(command=="isready"){
            cout<<"readyok"<<endl;
        }
        else if(command=="ucinewgame"){
            board=Board();
            prev_best=Move::NO_MOVE;
            for(int c=0;c<2;c++)
                for(int i=0;i<64;i++)
                    for(int j=0;j<64;j++)
            historyTable[c][i][j]=0;
            for(int i=0;i<64;i++)
                for(int j=0;j<2;j++)
                    killer[i][j]=Move::NO_MOVE;
            for(auto &entry:TT){
                entry.key=0;
                entry.depth=-1;
                entry.score=0;
                entry.bestMove=Move::NO_MOVE;
                entry.flag=EXACT;
            }
        }
        else if(command=="position startpos"){
            board=Board();
        }
        else if(command.find("position startpos moves")==0){
            board=Board();
            command.erase(0,string("position startpos moves ").size());
            stringstream ss(command);
            string moveString;
            while(ss>>moveString){
                Move move=uci::uciToMove(board,moveString);
                board.makeMove(move);
            }
        }
        else if(command.find("go depth")==0){
            stringstream ss(command);
            string go,depth;
            int d;
            ss>>go>>depth>>d;
            stop_search=false;
            nodes=0;
            time_allow=LLONG_MAX;
            Move best=Bestmove(board,d);
            cout<<"bestmove "<<uci::moveToUci(best)<<'\n'<<flush;
        }
        else if(command.find("go movetime")==0){
            stringstream ss(command);
            string go,movetime;
            ss>>go>>movetime>>time_allow;
            stop_search=false;
            nodes=0;
            start=chrono::steady_clock::now();
            now=start;
            elapsed=0;
            long long prev_elapsed=0;
            long long last_iteration=0;
            prev_best=Bestmove(board,1);
            int depth=2;
            Move best=prev_best;
            while(elapsed<=time_allow){
                if(elapsed+last_iteration*3>=time_allow){
                    best=prev_best;
                    break;
                }
                best=Bestmove(board,depth);
                if(!stop_search)
                    prev_best=best;
                else{
                    best=prev_best;
                    break;
                }
                depth++;
                now=chrono::steady_clock::now();
                elapsed=chrono::duration_cast<chrono::milliseconds>(now-start).count();
                last_iteration=elapsed-prev_elapsed;
                prev_elapsed=elapsed;
            }
            stop_search=false;
            cout<<"bestmove "<<uci::moveToUci(best)<<'\n'<<flush;
        }
        else if(command.find("go wtime")==0){
            stringstream ss(command);
            string token;
            long long w=0,b=0,moves=30;
            ss>>token;
            while(ss>>token){
                if(token=="wtime")
                    ss>>w;
                else if(token=="btime")
                    ss>>b;
                else if(token=="movestogo")
                    ss>>moves;
            }
            moves=max(1LL,moves);
            if(board.sideToMove()==Color::WHITE)
                time_allow=w/moves;
            else
                time_allow=b/moves;
            time_allow=time_allow*9/10;
            stop_search=false;
            nodes=0;
            start=chrono::steady_clock::now();
            now=start;
            elapsed=0;
            long long prev_elapsed=0;
            long long last_iteration=0;
            prev_best=Bestmove(board,1);
            int depth=2;
            Move best=prev_best;
            while(elapsed<=time_allow){
                if(elapsed+last_iteration*3>=time_allow){
                    best=prev_best;
                    break;
                }
                best=Bestmove(board,depth);
                if(!stop_search)
                    prev_best=best;
                else{
                    best=prev_best;
                    break;
                }
                depth++;
                now=chrono::steady_clock::now();
                elapsed=chrono::duration_cast<chrono::milliseconds>(now-start).count();
                last_iteration=elapsed-prev_elapsed;
                prev_elapsed=elapsed;
            }
            stop_search=false;
            cout<<"bestmove "<<uci::moveToUci(best)<<'\n'<<flush;
        }
        else if(command=="quit"){
            break;
        }
    }
}
