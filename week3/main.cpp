#include "chess.hpp"
#include <bits/stdc++.h>
using namespace std;
using namespace chess;

int evaluate(Board& board, bool maximizingPlayer){
    chess::Movelist movelist;
    chess::movegen::legalmoves<chess::movegen::MoveGenType::ALL>(movelist, board);
    if(movelist.size()==0){
        if(board.inCheck()){
            if(maximizingPlayer) return -1000000;
            else return 1000000;
        }
        else return 0;
    }
    return 0;
}
int alpha_beta(Board& board, int depth,int alpha,int beta,bool maximizingPlayer){
    if(depth==0)
       return evaluate(board, maximizingPlayer);
    chess::Movelist movelist;
    chess::movegen::legalmoves<chess::movegen::MoveGenType::ALL>(movelist, board);
    if(movelist.empty())
        return evaluate(board, maximizingPlayer);
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
Move Bestmove(Board& board,int depth){
    chess::Movelist movelist;
    chess::movegen::legalmoves<chess::movegen::MoveGenType::ALL>(movelist, board);
    int bestscore=INT_MIN;
    Move bestmove;
    if(movelist.empty())
        return Move::NO_MOVE;
    for (auto move:movelist){
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
int main(){
    string FEN;
    int n;
    getline(cin, FEN);
    cin>>n;
    Board board(FEN);
    int depth = 2*n - 1;
    auto best = Bestmove(board, depth);
    cout<<uci::moveToUci(best);
}
