#include <iostream>
#include <cstdlib>
#include <ctime>
#include <set>
#include <vector>

using namespace std;

int main() {
    srand(time(0));
    
    char playAgain = 'Y';
    
    while (playAgain == 'y' || playAgain == 'Y') {

        // Place 3 ducks at random positions (1-20), no duplicates
        set<int> ducks;
        while (ducks.size() < 3)
            ducks.insert(rand() % 20 + 1);

        cout << "\n========================\n";
        cout << "    DUCK SHOOTER v1.0   \n";
        cout << "========================\n";
        cout << "3 ducks are hiding in numbers 1 to 20.\n";
        cout << "Pick 3 numbers to shoot!\n";
        
        set<int> choices;
        int input;

        while (choices.size() < 3) {
            cout << "Shot " << choices.size () + 1 << " of 3 - Enter a number (1-20): ";
            cin >> input;
            if (cin.fail() || input < 1 || input > 20) {
                cin.clear();
                cin.ignore(1000, '\n');
                cout << " Invaild! Enter a number between 1 and 20. \n";
                continue;
            }
            if (choices.count(input)) {
                cout << " You already picked " << input << "! Choose another.\n ";
                continue;
            }

            choices.insert(input);
        } 
    
      // Results
      cout << "\n --- Results --- \n";
      int hits = 0;
      for (int shot : choices){
        if (ducks.count (shot)){
            cout << "  #" << shot << " HIT!  Duck down!\n";
            hits++;
        } else {
            cout << "  #" << shot << " MISS\n";
        }
      }

      // Show where ducks were hiding 
      cout << "\nDucks were hiding at: ";
      for (int d : ducks) cout << d << " ";
      cout << "\n";

      // Score summary 
      cout << "\nYou hit " << hits << " out 3 ducks. \n";
      if      (hits == 3) cout << "Perfect shot! All ducks down!\n";
      else if (hits == 2) cout << "Great shooting! Almost perfect!\n";
      else if (hits == 1) cout << "One duck hit. Keep practicing!\n";
      else                cout << "All missed! Better luck next time!\n";

      cout << "\nPlay again? (y/n): ";
      cin >> playAgain;
    }
    
    cout << "\nGood hunting! Goodbye. \n\n";
    return 0;
}