// TicTacToePart3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// campaign_tictactoe.cpp
// A campaign wrapper around your tic-tac-toe engine.
// Compile: g++ -std=c++17 campaign_tictactoe.cpp -o campaign_ttt

#include <iostream>
#include <limits>
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>
#include <random>
#include <chrono>
#include <functional>
using namespace std;

const int SIZE = 3;
char board[SIZE][SIZE];

struct Hero {
    string name;
    string archetype; // "Paladin" or "Alchemist"
    char mark; // player's mark on board
    int maxHP;
    int hp;
    int att;
    int def;
    int specialUses; // special uses per match
};

struct Enemy {
    string name;
    char mark;
    int hp;
    int att;
    int def;
    string abilityName;
    // ability function: takes hero reference and enemy reference, modifies stats or hp
    function<void(Hero&, Enemy&)> ability;
    // chance to use ability before/after a match (0-100)
    int abilityChance;
    // flag if ability already used (for some one-shot abilities)
    bool used = false;
};

// ---------- Random utilities ----------
std::mt19937 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());

int randInt(int a, int b) { // inclusive
    uniform_int_distribution<int> dist(a, b);
    return dist(rng);
}

// ---------- Board & basic ttt from your original code ----------
void initializeBoard() {
    char pos = '1';
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            board[i][j] = pos++;
}

void printBoard() {
    cout << "\nCurrent board:\n";
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            cout << " " << board[i][j];
            if (j < SIZE - 1) cout << " |";
        }
        cout << "\n";
        if (i < SIZE - 1) cout << "---+---+---\n";
    }
    cout << "\n";
}

bool getCoordinates(int cell, int& row, int& col) {
    if (cell < 1 || cell > 9) return false;
    row = (cell - 1) / SIZE;
    col = (cell - 1) % SIZE;
    return true;
}

// Detect winner by checking identical non-digit characters (our marks will be non-digit)
char checkWinner() {
    for (int i = 0; i < SIZE; ++i) {
        if (!isdigit(board[i][0]) && board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return board[i][0];
        if (!isdigit(board[0][i]) && board[0][i] == board[1][i] && board[1][i] == board[2][i])
            return board[0][i];
    }
    if (!isdigit(board[0][0]) && board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return board[0][0];
    if (!isdigit(board[0][2]) && board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return board[0][2];

    bool full = true;
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (isdigit(board[i][j]))
                full = false;

    if (full) return 'T';
    return ' ';
}

// helper to find empty cells
vector<int> availableCells() {
    vector<int> cells;
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (isdigit(board[i][j]))
                cells.push_back(i * SIZE + j + 1);
    return cells;
}

bool isAdjacent(int r1, int c1, int r2, int c2) {
    return abs(r1 - r2) <= 1 && abs(c1 - c2) <= 1 && !(r1 == r2 && c1 == c2);
}

// Special moves (carried from original): paladinShift and alchemistSwap, adjusted to work with digits/marks
void alchemistSwapBoardPositions(int a, int b) {
    int r1, c1, r2, c2;
    if (!getCoordinates(a, r1, c1) || !getCoordinates(b, r2, c2)) {
        cout << "Invalid cells!\n";
        return;
    }
    if (board[r1][c1] == board[r2][c2]) {
        cout << "Can't swap identical marks!\n";
        return;
    }
    if (isdigit(board[r1][c1])) {
        cout << "Cell " << a << " is empty.\n";
        return;
    }
    if (isdigit(board[r2][c2])) {
        cout << "Cell " << b << " is empty.\n";
        return;
    }
    swap(board[r1][c1], board[r2][c2]);
}

void paladinShiftBoardPosition(int src, int dest) {
    int r1, c1, r2, c2;
    if (!getCoordinates(src, r1, c1) || !getCoordinates(dest, r2, c2)) {
        cout << "Invalid cells!\n";
        return;
    }
    if (isdigit(board[r1][c1])) {
        cout << "Source is empty!\n";
        return;
    }
    if (!isdigit(board[r2][c2])) {
        cout << "Destination occupied!\n";
        return;
    }
    if (!isAdjacent(r1, c1, r2, c2)) {
        cout << "Destination must be adjacent!\n";
        return;
    }
    board[r2][c2] = board[r1][c1];
    board[r1][c1] = '0' + (src);
}

// A single match between hero and enemy. Returns 'H' if hero wins, 'E' if enemy wins, 'T' for tie.
char playMatch(Hero& hero, Enemy& enemy) {
    initializeBoard();
    char currentMark = 'X'; // we'll let hero be hero.mark and enemy mark set to enemy.mark
    // choose who goes first randomly
    bool heroTurn = (randInt(0, 1) == 0);

    // special uses reset per match
    hero.specialUses = (hero.archetype == "Paladin") ? 1 : 1; // 1 special action allowed per match for both for simplicity

    while (true) {
        printBoard();
        if (heroTurn) {
            cout << hero.name << " (you) HP: " << hero.hp << "  |  " << enemy.name << " HP: " << enemy.hp << "\n";
            cout << "Your move. Options:\n";
            cout << "1) Place mark\n";
            if (hero.specialUses > 0) {
                cout << "2) Use special (" << hero.archetype << ") [" << hero.specialUses << " left]\n";
            }
            cout << "Choose: ";
            int choice;
            if (!(cin >> choice)) {
                cout << "Invalid input, try again.\n";
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            }
            if (choice == 1) {
                int move;
                cout << "Enter move (1-9): ";
                cin >> move;
                int r, c;
                if (!getCoordinates(move, r, c) || !isdigit(board[r][c])) {
                    cout << "Invalid move!\n";
                    continue;
                }
                board[r][c] = hero.mark;
            }
            else if (choice == 2 && hero.specialUses > 0) {
                // Use special: Paladin shift or Alchemist swap
                if (hero.archetype == "Alchemist") {
                    cout << "Alchemist special: swap two occupied cells.\n";
                    int a, b; cout << "Enter two cell numbers to swap: "; cin >> a >> b;
                    alchemistSwapBoardPositions(a, b);
                    hero.specialUses--;
                }
                else { // Paladin
                    cout << "Paladin special: shift an occupied cell to adjacent empty cell.\n";
                    int src, dst; cout << "Enter source cell: "; cin >> src;
                    cout << "Enter destination cell: "; cin >> dst;
                    paladinShiftBoardPosition(src, dst);
                    hero.specialUses--;
                }
            }
            else {
                cout << "Invalid choice or no specials left.\n";
                continue;
            }
        }
        else {
            // Enemy random move
            vector<int> cells = availableCells();
            if (cells.empty()) {
                // board full
            }
            else {
                int pick = cells[randInt(0, (int)cells.size() - 1)];
                int r, c; getCoordinates(pick, r, c);
                board[r][c] = enemy.mark;
                cout << enemy.name << " placed on " << pick << ".\n";
            }
        }

        char w = checkWinner();
        if (w != ' ') {
            if (w == 'T') {
                cout << "Match ended in a tie.\n";
                return 'T';
            }
            if (w == hero.mark) {
                cout << hero.name << " wins the match!\n";
                return 'H';
            }
            else {
                cout << enemy.name << " wins the match!\n";
                return 'E';
            }
        }
        heroTurn = !heroTurn;
    }
}

// Damage application: damage = max(0, attacker.att - defender.def)
int calcDamage(int attacker, int defender) {
    int d = attacker - defender;
    return (d > 0) ? d : 0;
}

// ---------- Enemy factory (creates a list of enemies for campaign) ----------
vector<Enemy> createEnemies() {
    vector<Enemy> foes;

    // Goblin - small foe, ability: steal (reduces hero defense for next match)
    foes.push_back(Enemy{
        "Goblin Skulker", 'O',
        10,  // hp
        4,   // att
        1,   // def
        "Sneak Thief",
        [](Hero& h, Enemy& e) {
            cout << e.name << " uses Sneak Thief! Your defense is reduced by 1 for next match.\n";
            h.def = max(0, h.def - 1);
        }, 30
        });

    // Bandit - medium foe, ability: opportunistic strike (extra damage once)
    foes.push_back(Enemy{
        "Bandit Captain", 'B',
        14,
        6,
        2,
        "Opportunistic Strike",
        [](Hero& h, Enemy& e) {
            cout << e.name << " uses Opportunistic Strike! They deal 3 immediate damage to you.\n";
            int dmg = max(0, 3 - h.def);
            h.hp -= dmg;
            cout << "You took " << dmg << " damage.\n";
        }, 25
        });

    // Elemental - magic foe, ability: rust (lowers hero attack)
    foes.push_back(Enemy{
        "Corrupted Elemental", 'E',
        18,
        5,
        3,
        "Rust Burst",
        [](Hero& h, Enemy& e) {
            cout << e.name << " releases Rust Burst! Your attack lowered by 2 (not below 0).\n";
            h.att = max(0, h.att - 2);
        }, 30
        });

    // Warlord - powerful foe, ability: armor break (reduces hero defense and buff self)
    foes.push_back(Enemy{
        "Warlord of Ash", 'W',
        22,
        7,
        4,
        "Armor Break",
        [](Hero& h, Enemy& e) {
            cout << e.name << " uses Armor Break! Your defense -2; their defense +1.\n";
            h.def = max(0, h.def - 2);
            e.def += 1;
        }, 35
        });

    // Final boss - multiple abilities (will be used randomly)
    Enemy finalBoss;
    finalBoss.name = "The Dragon Sovereign";
    finalBoss.mark = 'D';
    finalBoss.hp = 40;
    finalBoss.att = 9;
    finalBoss.def = 5;
    finalBoss.abilityName = "Dragon's Wrath (multi)";
    finalBoss.abilityChance = 40;
    finalBoss.used = false;
    finalBoss.ability = [](Hero& h, Enemy& e) {
        int pick = randInt(1, 3);
        if (pick == 1) {
            cout << e.name << " breathes FIRE! Massive attack dealt.\n";
            int dmg = max(0, 8 - h.def);
            h.hp -= dmg;
            cout << "You took " << dmg << " damage from fire.\n";
        }
        else if (pick == 2) {
            cout << e.name << " spreads a molten SHIELD! Their defense increases by 3 temporarily.\n";
            e.def += 3;
        }
        else {
            cout << e.name << " unleashes a TERROR ROAR, reducing your attack by 3.\n";
            h.att = max(0, h.att - 3);
        }
        };
    foes.push_back(finalBoss);

    return foes;
}

// ---------- Between-battle events (at least 3), include at least one player choice ----------
void eventHealing(Hero& hero) {
    int heal = randInt(3, 8);
    hero.hp = min(hero.maxHP, hero.hp + heal);
    cout << "You found a soothing spring and healed " << heal << " HP. Current HP: " << hero.hp << "/" << hero.maxHP << "\n";
}

void eventStatBoost(Hero& hero) {
    int which = randInt(1, 2);
    if (which == 1) {
        hero.att += 1;
        cout << "You trained with a traveling swordsman. Attack +1 (now " << hero.att << ").\n";
    }
    else {
        hero.def += 1;
        cout << "You practiced guarding. Defense +1 (now " << hero.def << ").\n";
    }
}

void eventChoicePath(Hero& hero) {
    cout << "At a fork in the road you must choose a path:\n";
    cout << "A) Shadow Trail (risky): faster route, but dangerous. Reward: bigger gain if you win.\n";
    cout << "B) Safe Road (safe): slower, steady reward.\n";
    cout << "Choose A or B: ";
    char c; cin >> c;
    if (c == 'A' || c == 'a') {
        cout << "You take the Shadow Trail. Ambush! You fight a minor skirmish (auto resolved):\n";
        int chance = randInt(1, 100);
        if (chance <= 60) {
            cout << "You succeed and find a rare tonic: +5 HP and +2 attack.\n";
            hero.hp = min(hero.maxHP, hero.hp + 5);
            hero.att += 2;
        }
        else {
            cout << "You were hurt in the ambush. Lose 4 HP but gain a relic: +1 defense.\n";
            hero.hp = max(1, hero.hp - 4);
            hero.def += 1;
        }
    }
    else {
        cout << "You take the Safe Road. You rest and recover modestly: +3 HP, +1 defense.\n";
        hero.hp = min(hero.maxHP, hero.hp + 3);
        hero.def += 1;
    }
}

void eventTrader(Hero& hero, int& gold) {
    cout << "You encounter a traveling trader. (This is a small shop event)\n";
    cout << "You have " << gold << " gold.\n";
    cout << "1) Heal potion (5 gold) => +6 HP\n";
    cout << "2) Sharpening stone (7 gold) => +2 Attack\n";
    cout << "3) Reinforced padding (7 gold) => +2 Defense\n";
    cout << "4) Leave\n";
    cout << "Choose: ";
    int ch; cin >> ch;
    if (ch == 1 && gold >= 5) { gold -= 5; hero.hp = min(hero.maxHP, hero.hp + 6); cout << "Healed +6 HP.\n"; }
    else if (ch == 2 && gold >= 7) { gold -= 7; hero.att += 2; cout << "Attack +2.\n"; }
    else if (ch == 3 && gold >= 7) { gold -= 7; hero.def += 2; cout << "Defense +2.\n"; }
    else cout << "No purchase.\n";
}

// ---------- Campaign flow ----------
void showHeroStats(const Hero& h, int gold) {
    cout << "\n---- " << h.name << " [" << h.archetype << "] ----\n";
    cout << "HP: " << h.hp << "/" << h.maxHP << "  ATK: " << h.att << "  DEF: " << h.def << "  Gold: " << gold << "\n\n";
}

void campaign() {
    cout << "Welcome to the Campaign!\n";
    Hero hero;
    cout << "Enter your character name: ";
    getline(cin, hero.name);
    if (hero.name.empty()) {
        getline(cin, hero.name);
    }
    cout << "Choose class (Paladin/Alchemist): ";
    cin >> hero.archetype;
    while (hero.archetype != "Paladin" && hero.archetype != "Alchemist") {
        cout << "Invalid. Choose Paladin or Alchemist: ";
        cin >> hero.archetype;
    }
    // set starting stats based on class
    hero.mark = 'X';
    if (hero.archetype == "Paladin") {
        hero.maxHP = 30;
        hero.hp = hero.maxHP;
        hero.att = 6;
        hero.def = 4;
    }
    else {
        hero.maxHP = 26;
        hero.hp = hero.maxHP;
        hero.att = 7;
        hero.def = 2;
    }

    int gold = 10; // used by trader event (Choice 3 not selected but small shop included)
    cout << "\nYour journey begins: you are " << hero.name << ", a " << hero.archetype << ".\n";
    cout << "Objective: Survive 5 battles and defeat the Dragon Sovereign.\n";

    vector<Enemy> foes = createEnemies();
    // We'll pick the first 4 regular foes, and final boss last
    vector<Enemy> campaignFoes;
    campaignFoes.push_back(foes[0]); // Goblin
    campaignFoes.push_back(foes[1]); // Bandit
    campaignFoes.push_back(foes[2]); // Elemental
    campaignFoes.push_back(foes[3]); // Warlord
    campaignFoes.push_back(foes[4]); // Final boss

    int step = 0;
    // At least 3 events in between battles: we will do event after battle 1, after battle 2, after battle 3
    while (step < (int)campaignFoes.size()) {
        Enemy enemy = campaignFoes[step];
        cout << "\n=== Encounter " << (step + 1) << " ===\n";
        // story text progression
        if (step == 0) cout << "Beginning: A small hamlet asks for your help. A Goblin Skulker menaces the outskirts.\n";
        else if (step == 1) cout << "Word spreads: A Bandit Captain robs wayfarers along the road.\n";
        else if (step == 2) cout << "The land grows twisted: a Corrupted Elemental blocks your path.\n";
        else if (step == 3) cout << "The Warlord of Ash tries to claim your head near the ruined keep.\n";
        else cout << "Final chapter: You stand before the Dragon Sovereign in the scorched throne hall.\n";

        // Each battle is a loop of matches until someone's HP reaches 0
        bool heroDead = false;
        bool enemyDead = false;
        // reset enemy used flag maybe
        enemy.used = false;

        while (hero.hp > 0 && enemy.hp > 0) {
            // chance for enemy to trigger ability before match
            if (enemy.ability && randInt(1, 100) <= enemy.abilityChance) {
                cout << "\n>> " << enemy.name << " attempts to use a special ability...\n";
                enemy.ability(hero, enemy);
            }

            // play a match: hero vs enemy
            cout << "\nNew tic-tac-toe match vs " << enemy.name << "!\n";
            enemy.mark = (enemy.mark ? enemy.mark : 'O'); // ensure mark set
            hero.mark = 'X'; // hero mark

            char result = playMatch(hero, enemy);
            if (result == 'H') {
                // enemy takes damage equal to hero.att - enemy.def
                int dmg = calcDamage(hero.att, enemy.def);
                enemy.hp -= dmg;
                cout << "Your victory deals " << dmg << " damage to " << enemy.name << " (HP now " << max(0, enemy.hp) << ").\n";
                // reward some gold for a match victory (small)
                int g = randInt(1, 3); gold += g;
                cout << "You loot " << g << " gold from the field. (Total gold: " << gold << ")\n";
            }
            else if (result == 'E') {
                int dmg = calcDamage(enemy.att, hero.def);
                hero.hp -= dmg;
                cout << enemy.name << " deals " << dmg << " damage to you (HP now " << max(0, hero.hp) << ").\n";
            }
            else {
                cout << "No damage from a tie.\n";
            }

            if (hero.hp <= 0) heroDead = true;
            if (enemy.hp <= 0) enemyDead = true;

            // small pause in narration
            if (heroDead || enemyDead) break;
            cout << "The match ends. Next match begins unless someone falls.\n";
        }

        if (heroDead) {
            cout << "\nYou have fallen in battle.\n";
            cout << "---- GAME OVER ----\n";
            return; // campaign restarts if they run program again; per requirement "If player dies, they lose the game and the campaign restarts."
        }
        else {
            cout << "\nYou defeated " << enemy.name << "!\n";
            // reward: small HP restore and gold
            int rewardGold = randInt(3, 8);
            gold += rewardGold;
            hero.hp = min(hero.maxHP, hero.hp + 4); // small heal
            cout << "You gain " << rewardGold << " gold and recover 4 HP. (Gold: " << gold << ", HP: " << hero.hp << ")\n";
            // story progress and branching choices
            if (step == 0) {
                cout << "The villagers cheer. You may choose your route forward.\n";
                cout << "Choose your path: 1) Through the woods (chance to find useful items) 2) Along the river (safer)\n";
                int pick; cin >> pick;
                if (pick == 1) {
                    cout << "Through the woods: you find a small shrine. Your attack increases by 1.\n";
                    hero.att += 1;
                }
                else {
                    cout << "Along the river: you rest peacefully. You heal 3 HP.\n";
                    hero.hp = min(hero.maxHP, hero.hp + 3);
                }
            }
            else if (step == 1) {
                // event between battles (stat/event)
                cout << "Between battles you encounter an event.\n";
                int ev = randInt(1, 3);
                if (ev == 1) eventHealing(hero);
                else if (ev == 2) eventStatBoost(hero);
                else eventChoicePath(hero); // the player choice event (Requirement 6)
            }
            else if (step == 2) {
                // trader/shop event (small shop)
                cout << "After the Elemental, you meet a traveling trader.\n";
                eventTrader(hero, gold);
            }
            else if (step == 3) {
                cout << "Before the final confrontation you find an ancient armory.\n";
                cout << "You can choose one of: 1) +3 HP (permanent) 2) +2 ATT 3) +2 DEF. Choose: ";
                int c; cin >> c;
                if (c == 1) { hero.maxHP += 3; hero.hp += 3; cout << "Max HP increased by 3.\n"; }
                else if (c == 2) { hero.att += 2; cout << "Attack increased by 2.\n"; }
                else { hero.def += 2; cout << "Defense increased by 2.\n"; }
            }
        }

        step++;
        // show current hero stats
        showHeroStats(hero, gold);
    }

    // If loop finishes, player beat final boss
    cout << "\n***** CONGRATULATIONS! *****\n";
    cout << hero.name << " has defeated the Dragon Sovereign and completed the campaign!\n";
    cout << "Final stats:\n";
    showHeroStats(hero, gold);
}

// ---------- Main ----------
int main() {
    cout << "====== TIC-TAC-TOE CAMPAIGN ======\n";
    cout << "This program expands classic tic-tac-toe into a battle campaign.\n";
    while (true) {
        cout << "\nMain Menu:\n1) Start Campaign\n2) Quit\nChoose: ";
        int m; if (!(cin >> m)) break;
        if (m == 1) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // flush newline for campaign input
            campaign();
            cout << "\nReturn to main menu? (y/n): ";
            char r; cin >> r;
            if (r == 'n' || r == 'N') break;
        }
        else break;
    }
    cout << "Thanks for playing!\n";
    return 0;
}

