#include "chess.hpp"
#include <bits/stdc++.h>
#include<chrono>
using namespace std;
using namespace chess;

chrono::steady_clock::time_point start,now;
long long elapsed,time_allow;
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
int bishopTable[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};
int rookTable[64] = {
     0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};
int queenTable[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};
int kingTable[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};

int pieceValue(Piece piece){
        if(piece.type() ==PieceType::PAWN) return 100;
        else if(piece.type() == PieceType::KNIGHT) return 305;
        else if(piece.type() == PieceType::BISHOP) return 333;
        else if(piece.type() == PieceType::ROOK) return 560;
        else if(piece.type() == PieceType::QUEEN) return 950;
        return 0;
    }


int moveScore(Board& board, Move move){
    if(board.isCapture(move)){
        Piece victim =  board.at(move.to());
        Piece attacker = board.at(move.from());
        return 10*pieceValue(victim) - pieceValue(attacker);
    }
    board.makeMove(move);
    bool check = board.inCheck();
    board.unmakeMove(move);
    if(check)
        return 500;
    return 0;
}

int evaluate(Board& board){
    chess::Movelist movelist;
    chess::movegen::legalmoves<chess::movegen::MoveGenType::ALL>(movelist, board);
    if(movelist.size()==0){
        if(board.inCheck()){
            if(board.sideToMove() == Color::WHITE) return -1000000;
            else return 1000000;
        }
    }
    else{
        int score=0;
        if(board.sideToMove() == Color::WHITE)
            score += 3 * movelist.size();
        else
            score -= 3 * movelist.size();
        for(int i=0;i<64;i++){
            Square sq(i);
            Piece piece = board.at(sq);
            if(piece == Piece::NONE) continue;
            int value=0;
            if(piece.type() == PieceType::PAWN) value = 100 + pawnTable[sq.relative_square(piece.color()).index()];
            else if(piece.type() == PieceType::KNIGHT) value =305 + knightTable[sq.relative_square(piece.color()).index()];
            else if(piece.type() == PieceType::BISHOP) value = 333 + bishopTable[sq.relative_square(piece.color()).index()];
            else if(piece.type() == PieceType::ROOK) value = 563+ rookTable[sq.relative_square(piece.color()).index()];
            else if(piece.type() == PieceType::QUEEN) value = 950+ queenTable[sq.relative_square(piece.color()).index()];
            else if(piece.type() == PieceType::KING)  value = kingTable[sq.relative_square(piece.color()).index()];
            if(piece.color() == Color::WHITE)
                    score += value;
            else
                score -= value;
        }
        return score;
    }
    return 0;
}
int alpha_beta(Board& board, int depth,int alpha,int beta,bool maximizingPlayer){
    if(depth==0)
       return evaluate(board);
    chess::Movelist moves_help;
    chess::movegen::legalmoves<chess::movegen::MoveGenType::ALL>(moves_help, board);
    vector<Move> movelist;
    for(auto m : moves_help)  movelist.push_back(m);
    sort(movelist.begin(), movelist.end(), [&](Move a, Move b){
        return moveScore(board,a) > moveScore(board,b);});

    now = chrono::steady_clock::now();
    elapsed = chrono::duration_cast<chrono::milliseconds>(now - start).count();
    if(movelist.empty())
        return evaluate(board);
    if(elapsed>=time_allow){
        stop_search = true;
        return evaluate(board);
    }
    if(stop_search) return evaluate(board);
    if(maximizingPlayer){
        int utility=INT_MIN;
        for (auto move:movelist){
            board.makeMove(move);
            int eval=alpha_beta(board,depth-1,alpha,beta,false);
            utility=max(utility,eval);
            alpha=max(alpha,utility);
            board.unmakeMove(move);
            if (beta<=alpha)
                break;
    }
            return utility;
    }
    else{
        int utility=INT_MAX;
        for (auto move:movelist){
            board.makeMove(move);
            int eval=alpha_beta(board,depth-1,alpha,beta,true);
            utility=min(utility,eval);
            beta=min(beta,utility);
            board.unmakeMove(move);
            if (beta<=alpha)
                break;  
        }
        return utility;
    }
}
Move Bestmove(Board& board,int depth,bool maximizingPlayer){
    chess::Movelist movelist;
    chess::movegen::legalmoves<chess::movegen::MoveGenType::ALL>(movelist, board);
    if(maximizingPlayer){
        int bestscore=INT_MIN;
        Move bestmove;
        if(movelist.empty())
            return Move::NO_MOVE;
        for (auto move:movelist){
            if(stop_search) break;
            board.makeMove(move);
            int score=alpha_beta(board,depth-1,INT_MIN,INT_MAX,false);
            board.unmakeMove(move);
            if(score>bestscore){
                bestmove=move;
                bestscore=score;
            }
        }
        return bestmove;
    }
    else{
        int bestscore=INT_MAX;
        Move bestmove;
        if(movelist.empty())
            return Move::NO_MOVE;
        for (auto move:movelist){
            if(stop_search) break;
            board.makeMove(move);
            int score=alpha_beta(board,depth-1,INT_MIN,INT_MAX,true);
            board.unmakeMove(move);
            if(score<bestscore){
                bestmove=move;
                bestscore=score;
            }
        }
        return bestmove;
    }

}

int main() {
    string command;
    Board board;

    while (getline(cin, command)) {
        if (command == "uci") {
            cout << "id name CherylBot"<<endl;
            cout << "id author Cheryl"<<endl;
            cout << "uciok"<<endl;
        }

        else if (command == "isready") {
            cout << "readyok"<<endl;
        }
        else if(command == "position startpos"){
            board = Board();
        }
        else if(command.find("position startpos moves") == 0){
            board=Board();
            command.erase(0,string("position startpos moves ").size());
            stringstream ss(command);
            string moves_str=command;
            while(ss>>moves_str){
                Move move = uci::uciToMove(board, moves_str);
                board.makeMove(move);
            }
        }
        else if(command.find("go depth") == 0){
            stringstream ss(command);
            string go,depth;
            int no;
            ss>>go>>depth>>no;
            stop_search = false;
            time_allow = LLONG_MAX;
            Move best;
            if(board.sideToMove() == Color::WHITE)
                best = Bestmove(board, no,true);
            else
                best = Bestmove(board, no,false);
             cout <<"bestmove "<<uci::moveToUci(best)<<endl;
        }
        else if(command.find("go movetime") == 0){
            stringstream ss(command);
            string go,movetime;
            ss>>go>>movetime>>time_allow; 
            stop_search=false;
            start = chrono::steady_clock::now();
            now = chrono::steady_clock::now();
            elapsed =chrono::duration_cast<chrono::milliseconds>(now - start).count();
            int i=2;
            Move best= Move::NO_MOVE,prev_best;
            if(board.sideToMove() == Color::WHITE)
                prev_best=Bestmove(board,1,true);
            else 
                prev_best=Bestmove(board,1,false);
            stop_search=false;
            while(elapsed<=time_allow){  
                if(board.sideToMove() == Color::WHITE)
                    best = Bestmove(board,i,true);
                else
                    best = Bestmove(board,i,false);
                i++;
                if(!stop_search) prev_best=best;
                if(stop_search){
                    best=prev_best;
                    break;
                }
                now = chrono::steady_clock::now();
                elapsed =chrono::duration_cast<chrono::milliseconds>(now - start).count();
            }
            stop_search=false;
            cout <<"bestmove "<<uci::moveToUci(best)<<endl;
        }      
        else if(command.find("go wtime") == 0){
            stringstream ss(command);
            string go,wtime,btime,movestogo;
            long long  no_w,no_b,moves_no;
            ss>>go>>wtime>>no_w>>btime>>no_b>>movestogo>>moves_no; 
            if(board.sideToMove() == Color::WHITE) time_allow=no_w/moves_no ;
            else time_allow=no_b/moves_no ;
            stop_search=false;
            start = chrono::steady_clock::now();
            now = chrono::steady_clock::now();
            elapsed =chrono::duration_cast<chrono::milliseconds>(now - start).count();
            int i=2;
            Move best= Move::NO_MOVE,prev_best;
            if(board.sideToMove() == Color::WHITE)
                prev_best=Bestmove(board,1,true);
            else 
                prev_best=Bestmove(board,1,false);
            stop_search=false;
            while(elapsed<=time_allow){  
                if(board.sideToMove() == Color::WHITE)
                    best = Bestmove(board,i,true);
                else
                    best = Bestmove(board,i,false);
                i++;
                if(!stop_search) prev_best=best;
                if(stop_search){
                    best=prev_best;
                    break;
                }
                now = chrono::steady_clock::now();
                elapsed =chrono::duration_cast<chrono::milliseconds>(now - start).count();
            }
            stop_search=false;
            cout <<"bestmove "<<uci::moveToUci(best)<<endl;
        }
        else if (command == "quit") {
            break;
        }

    }
}
