import json
import copy  # use it for deepcopy if needed
import math  # for math.inf
import logging

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)

# Global variables in which you need to store player strategies (this is data structure that'll be used for evaluation)
# Mapping from histories (str) to probability distribution over actions
strategy_dict_x = {}
strategy_dict_o = {}
memo={}

class History:
    def __init__(self, history=None):
        """
        # self.history : Eg: [0, 4, 2, 5]
            keeps track of sequence of actions played since the beginning of the game.
            Each action is an integer between 0-8 representing the square in which the move will be played as shown
            below.
              ___ ___ ____
             |_0_|_1_|_2_|
             |_3_|_4_|_5_|
             |_6_|_7_|_8_|

        # self.board
            empty squares are represented using '0' and occupied squares are either 'x' or 'o'.
            Eg: ['x', '0', 'x', '0', 'o', 'o', '0', '0', '0']
            for board
              ___ ___ ____
             |_x_|___|_x_|
             |___|_o_|_o_|
             |___|___|___|

        # self.player: 'x' or 'o'
            Player whose turn it is at the current history/board

        :param history: list keeps track of sequence of actions played since the beginning of the game.
        """
        if history is not None:
            self.history = history
            self.board = self.get_board()
        else:
            self.history = []
            self.board = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
        self.player = self.current_player()

    def current_player(self):
        """ Player function
        Get player whose turn it is at the current history/board
        :return: 'x' or 'o' or None
        """
        total_num_moves = len(self.history)
        if total_num_moves < 9:
            if total_num_moves % 2 == 0:
                return 'x'
            else:
                return 'o'
        else:
            return None

    def get_board(self):
        """ Play out the current self.history and get the board corresponding to the history in self.board.

        :return: list Eg: ['x', '0', 'x', '0', 'o', 'o', '0', '0', '0']
        """
        board = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
        for i in range(len(self.history)):
            if i % 2 == 0:
                board[self.history[i]] = 'x'
            else:
                board[self.history[i]] = 'o'
        return board

    def is_win(self):
        # check if the board position is a win for either players
        # Feel free to implement this in anyway if needed
        # horizontal
        for i in range(0,9,3):
            if self.board[i]==self.board[i+1] and self.board[i+1]==self.board[i+2]:
                if self.board[i]=="x":
                    return "x"
                elif self.board[i]=="o":
                    return "o"
        # vertical
        for i in range(0,3,1):
            if self.board[i]==self.board[i+3] and self.board[i+3]==self.board[i+6]:
                if self.board[i]=="x":
                    return "x"
                elif self.board[i]=="o":
                    return "o"   
        # diagonal
        if self.board[0]==self.board[4] and self.board[4]==self.board[8]:
                if self.board[0]=="x":
                    return "x"
                elif self.board[0]=="o":
                    return "o" 
        # anti diagonal
        if self.board[2]==self.board[4] and self.board[4]==self.board[6]:
                if self.board[2]=="x":
                    return "x"
                elif self.board[2]=="o":
                    return "o"
        return "Not yet"         
               
    def is_draw(self):
        if "0" not in self.board and self.is_win()=="Not yet":
            return True
        return False
        # check if the board position is a draw
        # Feel free to implement this in anyway if needed

    def get_valid_actions(self):
        actions = []
        for i in range(9):
            if self.board[i] == "0":
                 actions.append(i)
        return actions
        # get the empty squares from the board
        # Feel free to implement this in anyway if needed

    def is_terminal_history(self):
        if self.is_win()=="x" or self.is_win()=="o" or self.is_draw()==True:
            return True
        return False
        # check if the history is a terminal history
        # Feel free to implement this in anyway if needed


    def get_utility_given_terminal_history(self):
        if self.is_terminal_history():
            if self.is_win()=="x":
                return 1
            elif self.is_win()=="o":
                return -1
            else:
                return 0
        # Feel free to implement this in anyway if needed

    def update_history(self, action):
        # In case you need to create a deepcopy and update the history obj to get the next history object.
        # Feel free to implement this in anyway if needed
        pass


def backward_induction(history_obj):
    """
    :param history_obj: Histroy class object
    :return: best achievable utility (float) for th current history_obj
    """
    global strategy_dict_x, strategy_dict_o,memo
    # TODO implement
    # (1) Implement backward induction for tictactoe
    # (2) Update the global variables strategy_dict_x or strategy_dict_o which are a mapping from histories to
    # probability distribution over actions.
    # (2a)These are dictionary with keys as string representation of the history list e.g. if the history list of the
    # history_obj is [0, 4, 2, 5], then the key is "0425". Each value is in turn a dictionary with keys as actions 0-8
    # (str "0", "1", ..., "8") and each value of this dictionary is a float (representing the probability of
    # choosing that action). Example: {"0452": {"0": 0, "1": 0, "2": 0, "3": 0, "4": 0, "5": 0, "6": 1, "7": 0, "8":
    # 0}}
    # (2b) Note, the strategy for each history in strategy_dict_x and strategy_dict_o is probability distribution over
    # actions. But since tictactoe is a PIEFG, there always exists an optimal deterministic strategy (SPNE). So your
    # policy will be something like this {"0": 1, "1": 0, "2": 0, "3": 0, "4": 0, "5": 0, "6": 0, "7": 0, "8": 0} where
    # "0" was the one of the best actions for the current player/history.
    # TODO implement
    his_str="".join(str(x) for x in history_obj.history)
    if his_str in memo:
        return memo[his_str]
    prob_dis_o={"0": 0, "1": 0, "2": 0, "3": 0, "4": 0, "5": 0, "6": 0, "7": 0, "8":0}
    prob_dis_x={"0": 0, "1": 0, "2": 0, "3": 0, "4": 0, "5": 0, "6": 0, "7": 0, "8":0}
    if history_obj.is_terminal_history():
        memo[his_str] = history_obj.get_utility_given_terminal_history()
        return history_obj.get_utility_given_terminal_history()
    if history_obj.current_player()=="x":
        diff_x=history_obj.get_valid_actions()
        utility=float('-inf')
        action_value={}
        for i in diff_x:
            new_history = copy.deepcopy(history_obj)
            new_history.history.append(i)
            new_history.board = new_history.get_board()
            action_value[i]=backward_induction(new_history)
            utility=max(utility,action_value[i])
        index=[]
        for i in diff_x:
            if utility==action_value[i]:
                index.append(i)
        for i in index:
            prob_dis_x[f"{i}"]=1.0/len(index)
        strategy_dict_x[his_str]=prob_dis_x
        memo[his_str] = utility
        return utility   
    elif history_obj.current_player()=="o":
        diff_o=history_obj.get_valid_actions()
        utility=float('inf')
        index=[]
        action_value={}
        for i in diff_o:
            new_history = copy.deepcopy(history_obj)
            new_history.history.append(i)
            new_history.board = new_history.get_board()
            action_value[i]=backward_induction(new_history)
            utility=min(utility,action_value[i])
        for i in diff_o:
            if utility==action_value[i]:
                index.append(i)
        for i in index:
            prob_dis_o[f"{i}"]=1.0/len(index)
        strategy_dict_o[his_str]=prob_dis_o
        memo[his_str] = utility
        return utility 
    

def solve_tictactoe():
    backward_induction(History())
    with open('./policy_x.json', 'w') as f:
        json.dump(strategy_dict_x, f)
    with open('./policy_o.json', 'w') as f:
        json.dump(strategy_dict_o, f)
    return strategy_dict_x, strategy_dict_o


if __name__ == "__main__":
    logging.info("Start")
    solve_tictactoe()
    logging.info("End")
