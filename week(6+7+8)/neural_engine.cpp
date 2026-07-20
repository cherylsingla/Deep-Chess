#include "chess.hpp"
#include <bits/stdc++.h>
#include<chrono>
#include "nn_weights.h"
using namespace std;
using namespace chess;

chrono::steady_clock::time_point start,now;
long long elapsed,time_allow;
long long nodes = 0;
Move killer[64][2];
int historyTable[2][64][64];
Move prev_best = Move::NO_MOVE;
bool stop_search=false;
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
constexpr int INPUT_SIZE=773;
constexpr int H1_SIZE=512;
constexpr int H2_SIZE=256;
constexpr int H3_SIZE=64;
vector<TTEntry> TT(TT_SIZE);

inline float relu(float x){
    return (x > 0.0f) ? x : 0.0f;
}

inline int featureIndex(PieceType type, Color color){
    if(color==Color::WHITE){
        if(type==PieceType::PAWN) return 0;
        else if(type==PieceType::KNIGHT) return 64;
        else if(type==PieceType::BISHOP) return 128;
        else if(type==PieceType::ROOK) return 192;
        else if(type==PieceType::QUEEN) return 256;
        else if(type==PieceType::KING)  return 320;
    }
    else{
        if(type==PieceType::PAWN) return 384;
        else if(type==PieceType::KNIGHT) return 448;
        else if(type==PieceType::BISHOP) return 512;
        else if(type==PieceType::ROOK)  return 576;
        else if(type==PieceType::QUEEN)  return 640;
        else if(type==PieceType::KING)   return 704;
    }
    return -1;
}

void boardToFeatures(Board &board,float input[INPUT_SIZE]){

    for(int i=0;i<INPUT_SIZE;i++)
        input[i]=0.0f;
    for(int sq=0;sq<64;sq++){
        Piece piece=board.at(Square(sq));
        if(piece==Piece::NONE) continue;
        int base=featureIndex(piece.type(),piece.color());
        if(base==-1)
            continue;
        input[base+sq]=1.0f;
    }

    input[768]=(board.sideToMove()==Color::WHITE)?1.0f:0.0f;
    if(board.castlingRights().has(Color::WHITE, Board::CastlingRights::Side::KING_SIDE)) input[769] = 1.0f;
    if(board.castlingRights().has(Color::WHITE, Board::CastlingRights::Side::QUEEN_SIDE)) input[770] = 1.0f;
    if(board.castlingRights().has(Color::BLACK, Board::CastlingRights::Side::KING_SIDE)) input[771] = 1.0f;
    if(board.castlingRights().has(Color::BLACK, Board::CastlingRights::Side::QUEEN_SIDE)) input[772] = 1.0f;
}

float forward(float input[INPUT_SIZE]){

    float h1[H1_SIZE];
    float h2[H2_SIZE];
    float h3[H3_SIZE];
    for(int i=0;i<H1_SIZE;i++){
        float sum=FC1_BIAS[i];
        for(int j=0;j<INPUT_SIZE;j++) sum+=FC1_WEIGHT[i][j]*input[j];
        h1[i]=relu(sum);
    }
    for(int i=0;i<H2_SIZE;i++){
        float sum=FC2_BIAS[i];
        for(int j=0;j<H1_SIZE;j++) sum+=FC2_WEIGHT[i][j]*h1[j];
        h2[i]=relu(sum);
    }

    for(int i=0;i<H3_SIZE;i++){
        float sum=FC3_BIAS[i];
        for(int j=0;j<H2_SIZE;j++) sum+=FC3_WEIGHT[i][j]*h2[j];
        h3[i]=relu(sum);
    }
    float output=FC4_BIAS[0];
    for(int j=0;j<H3_SIZE;j++) output+=FC4_WEIGHT[0][j]*h3[j];
    return output;
}

int neuralEvaluate(Board &board){
    float input[INPUT_SIZE];
    boardToFeatures(board,input);
    float score=forward(input);
    return static_cast<int>(round(score*1500.0f));
}

int evaluate(Board &board){
    return neuralEvaluate(board);
}

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
    if(board.isRepetition()) return 0;
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
