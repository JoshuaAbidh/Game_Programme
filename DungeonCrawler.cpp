// ============================================================
//  INFINITE DUNGEON CRAWLER  v3
//  COMP1601 / COMP1602 level C++
//  NEW: Assassination system + Expanded combat options
//       (Distract, Taunt, Power Strike, Defend, Observe,
//        Assassinate, Steal, Class Ability)
// ============================================================

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <fstream>

using namespace std;

// ============================================================
//  CONSTANTS
// ============================================================
const int MAX_INVENTORY  = 20;
const int MAX_BUFFS      = 20;
const int MAX_ITEMS      = 50;
const int MAX_SAVE_SLOTS = 5;          // scan up to 5 save files
const string SAVE_PREFIX = "save_";   // files: save_1.dat, save_2.dat ...
const string SAVE_EXT    = ".dat";

// ============================================================
//  ENUMS
// ============================================================
enum ClassType { KNIGHT, PALADIN, MAGE, DEMON, SKELETON, SHAPESHIFTER, CLASS_COUNT };
enum ItemType  { WEAPON, ARMOUR, POTION, RELIC };

// ============================================================
//  STRUCTS
// ============================================================
struct Item {
    string   name;
    ItemType type;
    int      value;
    bool     active;
};

struct Buff {
    string name;
    string description;
    int    atkBonus;
    int    defBonus;
    int    hpBonus;
    bool   active;
};

struct Character {
    string    name;
    ClassType cls;
    int       hp, maxHp;
    int       atk, def;
    int       gold;
    int       floor;
    Item      inventory[MAX_INVENTORY];
    int       invSize;
    Buff      buffs[MAX_BUFFS];
    int       buffCount;
    bool      disguised;
    ClassType disguiseAs;
    // base stats (saved separately so scaling stays clean)
    int       baseAtk, baseDef, baseMaxHp;

    // === Combat extras ===
    int       defendBonus;   // temporary defence from Defend action (resets each combat)
    bool      abilityUsed;   // class ability used this combat (1 per fight)
    int       stealCooldown; // turns until Steal can be used again
};

struct Enemy {
    string    name;
    ClassType cls;
    int       hp, maxHp;
    int       atk, def;
    bool      isBoss;
    int       tier;          // difficulty tier (1-5)

    // === Assassination / combat state ===
    bool      distracted;    // set by Distract action -- enables Assassinate
    int       distractTurns; // how many turns distraction lasts
    bool      stunned;       // skips enemy attack this turn
    bool      defDebuff;     // enemy defence has been lowered (Expose)
    int       defDebuffAmt;  // how much def was lowered
    bool      observed;      // player studied enemy (reveals weakness hint)
    int       weaknessMod;   // bonus dmg when weakness exploited
};

struct NPC {
    string name;
    ClassType cls;
    string greeting;
    Item   shop[3];
    int    shopSize;
};

// ============================================================
//  ITEM DATABASE
// ============================================================
Item ITEM_DB[MAX_ITEMS];
int  ITEM_DB_SIZE = 0;

void buildItemDB(){
    string wNames[15]={
        "Rusty Sword","Iron Blade","Shadow Dagger","Flame Sword","Holy Mace",
        "Bone Staff","Dark Wand","Crystal Spear","Thunder Axe","Venom Bow",
        "Silver Lance","Chaos Blade","Dragon Fang","Soul Reaper","Void Edge"};
    int wVal[15]={3,5,6,10,8,7,9,11,12,8,9,14,15,16,18};
    for(int i=0;i<15;i++){
        ITEM_DB[ITEM_DB_SIZE]={wNames[i],WEAPON,wVal[i],true}; ITEM_DB_SIZE++;}

    string aNames[15]={
        "Leather Vest","Chain Mail","Iron Shield","Bone Armour","Mage Robe",
        "Shadow Cloak","Paladin Plate","Demon Hide","Crystal Vest","Dragon Scale",
        "Holy Barrier","Void Cloak","Thunder Guard","Frost Mail","Death Shroud"};
    int aVal[15]={2,4,5,4,3,5,8,6,7,12,9,10,11,8,13};
    for(int i=0;i<15;i++){
        ITEM_DB[ITEM_DB_SIZE]={aNames[i],ARMOUR,aVal[i],true}; ITEM_DB_SIZE++;}

    string pNames[10]={
        "Small Potion","Large Potion","Mega Potion","Elixir",
        "Antidote","Mana Brew","Blood Vial","Holy Water","Shadow Draught","Dragon Brew"};
    int pVal[10]={15,30,50,80,20,25,35,45,40,100};
    for(int i=0;i<10;i++){
        ITEM_DB[ITEM_DB_SIZE]={pNames[i],POTION,pVal[i],true}; ITEM_DB_SIZE++;}

    string rNames[10]={
        "Amulet of Strength","Ring of Defence","Orb of Fate","Dragon Tooth",
        "Shadow Gem","Holy Relic","Cursed Idol","Void Shard","Phoenix Feather","Demon Eye"};
    int rVal[10]={5,5,7,6,6,8,9,10,12,14};
    for(int i=0;i<10;i++){
        ITEM_DB[ITEM_DB_SIZE]={rNames[i],RELIC,rVal[i],true}; ITEM_DB_SIZE++;}
}

// ============================================================
//  BUFF DATABASE  (20 buffs)
// ============================================================
Buff BUFF_DB[MAX_BUFFS]={
    {"Berserker Rage",   "ATK +5, DEF -1",   5,-1, 0,true},
    {"Stone Skin",       "DEF +5",            0, 5, 0,true},
    {"Battle Cry",       "ATK +3, HP +10",    3, 0,10,true},
    {"Shadow Step",      "DEF +4",            0, 4, 0,true},
    {"Holy Blessing",    "HP +25",            0, 0,25,true},
    {"Cursed Strength",  "ATK +7, HP -10",    7, 0,-10,true},
    {"Iron Will",        "DEF +3, HP +15",    0, 3,15,true},
    {"Arcane Surge",     "ATK +6",            6, 0, 0,true},
    {"Vampiric Touch",   "ATK +4, HP +10",    4, 0,10,true},
    {"Dragon Blood",     "ATK +4, DEF +4",    4, 4, 0,true},
    {"Phantom Guard",    "DEF +6",            0, 6, 0,true},
    {"Thunder Body",     "ATK +5, DEF +2",    5, 2, 0,true},
    {"Frost Aura",       "DEF +4, HP +5",     0, 4, 5,true},
    {"Demon Pact",       "ATK +8, DEF -2",    8,-2, 0,true},
    {"Spectral Form",    "DEF +5, ATK +2",    2, 5, 0,true},
    {"Divine Light",     "HP +30, DEF +2",    0, 2,30,true},
    {"Savage Instinct",  "ATK +6, DEF -1",    6,-1, 0,true},
    {"Undying Will",     "HP +40",            0, 0,40,true},
    {"Void Embrace",     "ATK +5, HP +10",    5, 0,10,true},
    {"Titan's Might",    "ATK +8, DEF +3",    8, 3, 0,true}
};

// ============================================================
//  NAME POOLS
// ============================================================
string NPC_NAMES[20]={
    "Alaric","Brenna","Caspian","Delara","Edwyn",
    "Fyra","Galdric","Hestia","Iorek","Jorath",
    "Kessa","Lumir","Morryn","Nadira","Oswin",
    "Peryn","Quorra","Raveth","Solan","Tyara"};
string ENEMY_NAMES[20]={
    "Grim Shade","Iron Fang","Bone Crusher","Void Walker","Shadow Stalker",
    "Plague Bearer","Stone Golem","Ash Knight","Hellhound","Frost Wraith",
    "Ember Drake","Cursed Monk","Grave Warden","Thorn Beast","Dusk Fiend",
    "Blight Lord","Chaos Imp","Ruin Seeker","Dark Elf","Soul Drinker"};
string BOSS_NAMES[10]={
    "The Dread Lich","Infernal Tyrant","Colossus of Ruin","Voidheart Serpent",
    "Demon King Vareth","The Undying One","Shadow Sovereign","Titan of Despair",
    "Cursed Overlord","The Final Dark"};

// ============================================================
//  HELPERS
// ============================================================
string className(ClassType c){
    switch(c){
        case KNIGHT:       return "Knight";
        case PALADIN:      return "Paladin";
        case MAGE:         return "Mage";
        case DEMON:        return "Demon";
        case SKELETON:     return "Skeleton";
        case SHAPESHIFTER: return "Shapeshifter";
        default:           return "Unknown";
    }
}
string itemTypeName(ItemType t){
    switch(t){
        case WEAPON: return "WPN";
        case ARMOUR: return "ARM";
        case POTION: return "POT";
        case RELIC:  return "REL";
        default:     return "???";
    }
}

void printLine(char c='=', int len=55){
    for(int i=0;i<len;i++) cout<<c;
    cout<<"\n";
}
void pause(){
    cout<<"\n[Press ENTER to continue]";
    cin.ignore();
    cin.get();
}
int rollDice(int sides){ return (rand()%sides)+1; }

// ============================================================
//  DIFFICULTY SCALING
//  Enemies scale in 5 tiers based on floor depth.
//  HP, ATK, DEF all grow with floor, with tier multipliers.
//
//  Tier 1:  floors  1-19   (Fledgling)
//  Tier 2:  floors 20-39   (Hardened)
//  Tier 3:  floors 40-59   (Veteran)
//  Tier 4:  floors 60-79   (Elite)
//  Tier 5:  floors 80+     (Legendary)
// ============================================================
struct ScaleFactor {
    float hpMult;
    float atkMult;
    float defMult;
    string tierName;
};

ScaleFactor getTier(int floor){
    if(floor < 20)  return {1.0f, 1.0f, 1.0f, "Fledgling"};
    if(floor < 40)  return {1.5f, 1.4f, 1.3f, "Hardened"};
    if(floor < 60)  return {2.2f, 1.9f, 1.7f, "Veteran"};
    if(floor < 80)  return {3.0f, 2.6f, 2.3f, "Elite"};
                    return {4.2f, 3.5f, 3.0f, "Legendary"};
}

// Flat stat bonus per floor (on top of multiplier)
int floorAtkBonus(int floor)  { return floor / 4; }
int floorDefBonus(int floor)  { return floor / 6; }
int floorHpBonus(int floor)   { return floor * 3; }

// ============================================================
//  ENEMY GENERATION  (with scaling)
// ============================================================
Enemy makeEnemy(int floor, bool boss){
    Enemy e;
    ScaleFactor sf = getTier(floor);
    e.isBoss = boss;
    e.cls    = (ClassType)(rand() % CLASS_COUNT);
    e.tier   = (floor<20)?1:(floor<40)?2:(floor<60)?3:(floor<80)?4:5;

    if(boss){
        int nameIdx   = ((floor/10)-1) % 10;
        e.name        = BOSS_NAMES[nameIdx];
        int baseHp    = 80  + floorHpBonus(floor);
        int baseAtk   = 10  + floorAtkBonus(floor);
        int baseDef   = 5   + floorDefBonus(floor);
        e.maxHp = (int)(baseHp  * sf.hpMult  * 1.5f);
        e.atk   = (int)(baseAtk * sf.atkMult * 1.3f);
        e.def   = (int)(baseDef * sf.defMult * 1.2f);
    } else {
        e.name        = ENEMY_NAMES[rand() % 20];
        int baseHp    = 20  + floorHpBonus(floor);
        int baseAtk   = 12  + floorAtkBonus(floor);   // raised from 8, matches avg player+weapon
        int baseDef   = 4   + floorDefBonus(floor);   // raised from 2, makes player offence meaningful but not trivial
        e.maxHp = (int)(baseHp  * sf.hpMult);
        e.atk   = (int)(baseAtk * sf.atkMult);
        e.def   = (int)(baseDef * sf.defMult);
    }
    e.hp = e.maxHp;
    if(e.atk < 2)  e.atk = 2;
    if(e.def < 0)  e.def = 0;
    if(e.maxHp<5)  e.maxHp = e.hp = 5;

    // Initialise combat state
    e.distracted   = false;
    e.distractTurns= 0;
    e.stunned      = false;
    e.defDebuff    = false;
    e.defDebuffAmt = 0;
    e.observed     = false;
    e.weaknessMod  = rollDice(6) + 2; // hidden until observed
    return e;
}

// ============================================================
//  INVENTORY
// ============================================================
void showInventory(const Character &ch){
    printLine();
    cout<<"  INVENTORY  ("<<ch.invSize<<"/"<<MAX_INVENTORY<<")\n";
    printLine();
    if(ch.invSize==0){ cout<<"  (empty)\n"; return; }
    for(int i=0;i<ch.invSize;i++){
        cout<<"  ["<<i+1<<"] "<<ch.inventory[i].name
            <<"  ["<<itemTypeName(ch.inventory[i].type)<<"]"
            <<"  val:"<<ch.inventory[i].value<<"\n";
    }
}

bool addItem(Character &ch, Item it){
    if(ch.invSize>=MAX_INVENTORY){ cout<<"  Inventory full!\n"; return false; }
    ch.inventory[ch.invSize++]=it;
    cout<<"  Got: "<<it.name<<"\n";
    return true;
}

void usePotion(Character &ch){
    if(ch.invSize==0){ cout<<"  No items.\n"; return; }
    showInventory(ch);
    cout<<"  Use which item? (0=cancel): ";
    int choice; cin>>choice;
    if(choice<=0||choice>ch.invSize) return;
    Item &it=ch.inventory[choice-1];
    if(it.type==POTION){
        ch.hp+=it.value;
        if(ch.hp>ch.maxHp) ch.hp=ch.maxHp;
        cout<<"  Used "<<it.name<<"! Restored "<<it.value<<" HP.\n";
        for(int i=choice-1;i<ch.invSize-1;i++) ch.inventory[i]=ch.inventory[i+1];
        ch.invSize--;
    } else {
        cout<<"  Can only use potions mid-dungeon.\n";
    }
}

void dropItem(Character &ch){
    if(ch.invSize==0){ cout<<"  Nothing to drop.\n"; return; }
    showInventory(ch);
    cout<<"  Drop which? (0=cancel): ";
    int choice; cin>>choice;
    if(choice<=0||choice>ch.invSize) return;
    cout<<"  Dropped: "<<ch.inventory[choice-1].name<<"\n";
    for(int i=choice-1;i<ch.invSize-1;i++) ch.inventory[i]=ch.inventory[i+1];
    ch.invSize--;
}

// ============================================================
//  BUFFS
// ============================================================
void showBuffs(const Character &ch){
    printLine();
    cout<<"  ACTIVE BUFFS\n";
    printLine();
    if(ch.buffCount==0){ cout<<"  (none)\n"; return; }
    for(int i=0;i<ch.buffCount;i++)
        cout<<"  * "<<ch.buffs[i].name<<" - "<<ch.buffs[i].description<<"\n";
}

void applyRandomBuff(Character &ch){
    if(ch.buffCount>=MAX_BUFFS){ cout<<"  Max buffs reached!\n"; return; }
    int idx=rand()%MAX_BUFFS;
    Buff b=BUFF_DB[idx];
    ch.buffs[ch.buffCount++]=b;
    ch.atk   +=b.atkBonus;
    ch.def   +=b.defBonus;
    ch.maxHp +=b.hpBonus;
    if(ch.def<0)   ch.def=0;
    if(ch.maxHp<1) ch.maxHp=1;
    if(b.hpBonus>0){ ch.hp+=b.hpBonus; if(ch.hp>ch.maxHp) ch.hp=ch.maxHp; }
    cout<<"\n  *** BUFF ACQUIRED: "<<b.name<<" ***\n";
    cout<<"  "<<b.description<<"\n";
}

// ============================================================
//  DICE ROLL EVENT  (every 5 floors)
// ============================================================
void diceRollEvent(Character &ch){
    printLine('*');
    cout<<"  *** MYSTICAL DICE ROLL EVENT! ***\n";
    printLine('*');
    cout<<"  You stand before an ancient altar...\n";
    cout<<"  Roll the dice of fate? (1=Yes / 2=No): ";
    int c; cin>>c;
    if(c!=1){ cout<<"  You walk away.\n"; return; }

    int roll=rollDice(20);
    cout<<"\n  You rolled a "<<roll<<"!\n";

    if(roll>=15){
        cout<<"  GREAT FORTUNE! Buff + item + gold!\n";
        applyRandomBuff(ch);
        int idx=rand()%ITEM_DB_SIZE;
        addItem(ch,ITEM_DB[idx]);
        ch.gold+=30;
        cout<<"  +30 gold!\n";
    } else if(roll>=10){
        cout<<"  GOOD FORTUNE! Random buff!\n";
        applyRandomBuff(ch);
        ch.gold+=15;
        cout<<"  +15 gold!\n";
    } else if(roll>=6){
        cout<<"  MODEST FORTUNE! A random item falls.\n";
        addItem(ch,ITEM_DB[rand()%ITEM_DB_SIZE]);
    } else if(roll>=3){
        int dmg=rollDice(10)+5;
        cout<<"  BAD OMEN! Cursed for "<<dmg<<" damage!\n";
        ch.hp-=dmg; if(ch.hp<0) ch.hp=0;
    } else {
        int dmg=rollDice(15)+10;
        cout<<"  TERRIBLE OMEN! Dark energy blasts you for "<<dmg<<"!\n";
        ch.hp-=dmg; if(ch.hp<0) ch.hp=0;
    }
}

// ============================================================
//  COMBAT HELPERS
// ============================================================
int getWeaponBonus(const Character &ch){
    int best=0;
    for(int i=0;i<ch.invSize;i++)
        if(ch.inventory[i].type==WEAPON && ch.inventory[i].value>best)
            best=ch.inventory[i].value;
    return best;
}
int getArmourBonus(const Character &ch){
    int best=0;
    for(int i=0;i<ch.invSize;i++)
        if(ch.inventory[i].type==ARMOUR && ch.inventory[i].value>best)
            best=ch.inventory[i].value;
    return best;
}
// Damage formula:
//   raw = atk + diceRoll - (def * 0.5)
// Defence reduces but never fully cancels damage.
// Minimum 1 always lands.
// Both player and enemy use the same formula -- no hidden advantage.
int calcDamage(int atk, int def, int diceRoll){
    int reducedDef = (int)(def * 0.5f);
    int raw = atk + diceRoll - reducedDef;
    return (raw < 1) ? 1 : raw;
}
// Overload for cases where we just want a standard d8 roll internally
int calcDamage(int atk, int def){
    return calcDamage(atk, def, rollDice(8));
}

// ============================================================
//  EXPANDED COMBAT SYSTEM  v3
//
//  [1] Normal Attack      standard hit
//  [2] Power Strike       2x ATK, costs 5 HP
//  [3] Defend             +8 DEF this round + weak counter
//  [4] Distract           set enemy distracted (enables Assassinate)
//  [5] Assassinate        instant kill if distracted (normal enemy)
//                         or 3.5x dmg on distracted boss
//  [6] Observe            reveal weakness, next hit gets bonus dmg
//  [7] Taunt              bait enemy into free attack, counter hits
//  [8] Steal              steal gold/item (easier when distracted)
//  [9] Use Item
//  [A] Class Ability      unique per class, once per combat
//  [0] Flee
// ============================================================

string DISTRACT_LINES[8]={
    "You toss a loose coin -- it clatters in the shadows!",
    "You mimic a distant scream -- the enemy spins around!",
    "You knock over a pile of bones -- the noise startles them!",
    "You whistle a haunting tune -- the enemy tilts its head, confused.",
    "You hurl a stone down the corridor -- they lurch toward the sound!",
    "You fake a surrendering bow -- the enemy hesitates, curious.",
    "You conjure a flicker of light behind them -- they turn to look!",
    "You drop to the ground -- the enemy searches above, baffled."
};

string ASSASSINATE_LINES[6]={
    "You dart from the shadows and drive your blade through their neck!",
    "A single, silent strike -- they collapse before uttering a sound.",
    "You plunge deep into the gap in their armour -- it's over instantly.",
    "Lightning fast -- one motion, one kill. Flawless.",
    "You slide behind them and end it before they can react.",
    "The darkness was your ally -- they never saw you coming."
};

string classAbilityName(ClassType c){
    switch(c){
        case KNIGHT:       return "Shield Bash   (stun + 150% dmg)";
        case PALADIN:      return "Holy Smite    (heal 20% HP + hit)";
        case MAGE:         return "Arcane Burst  (3x ATK, ignores DEF)";
        case DEMON:        return "Blood Fury    (spend 25% HP for 4x ATK)";
        case SKELETON:     return "Bone Rattle   (terrify: -4 enemy ATK)";
        case SHAPESHIFTER: return "Mimic Strike  (hit with enemy's own ATK)";
        default:           return "Unknown Ability";
    }
}

void doClassAbility(Character &ch, Enemy &en){
    cout<<"\n  *** CLASS ABILITY: ";
    switch(ch.cls){
        case KNIGHT:{
            cout<<"SHIELD BASH! ***\n";
            int dmg=(int)((ch.atk+getWeaponBonus(ch))*1.5f);
            en.hp-=dmg; en.stunned=true;
            cout<<"  You slam your shield for "<<dmg<<" dmg!\n";
            cout<<"  "<<en.name<<" is STUNNED and loses their next action!\n";
            break;
        }
        case PALADIN:{
            cout<<"HOLY SMITE! ***\n";
            int heal=ch.maxHp/5;
            int dmg=ch.atk+getWeaponBonus(ch)+rollDice(8);
            ch.hp+=heal; if(ch.hp>ch.maxHp) ch.hp=ch.maxHp;
            en.hp-=dmg;
            cout<<"  Divine energy heals you for "<<heal<<" HP!\n";
            cout<<"  A holy bolt strikes "<<en.name<<" for "<<dmg<<" dmg!\n";
            break;
        }
        case MAGE:{
            cout<<"ARCANE BURST! ***\n";
            int dmg=(ch.atk+getWeaponBonus(ch))*3;
            en.hp-=dmg;
            cout<<"  Arcane energy obliterates "<<en.name<<" for "<<dmg<<" dmg (ignores DEF)!\n";
            break;
        }
        case DEMON:{
            cout<<"BLOOD FURY! ***\n";
            int cost=ch.maxHp/4;
            int dmg=(ch.atk+getWeaponBonus(ch))*4;
            ch.hp-=cost; if(ch.hp<1) ch.hp=1;
            en.hp-=dmg;
            cout<<"  You sacrifice "<<cost<<" HP and unleash hellfire!\n";
            cout<<"  "<<en.name<<" takes "<<dmg<<" dmg!\n";
            break;
        }
        case SKELETON:{
            cout<<"BONE RATTLE! ***\n";
            int atkDrop=4;
            en.atk-=atkDrop; if(en.atk<1) en.atk=1;
            cout<<"  Your bones clatter horrifyingly!\n";
            cout<<"  "<<en.name<<"'s ATK reduced by "<<atkDrop<<"!\n";
            break;
        }
        case SHAPESHIFTER:{
            cout<<"MIMIC STRIKE! ***\n";
            int dmg=en.atk+rollDice(8);
            en.hp-=dmg;
            cout<<"  You mirror "<<en.name<<"'s own fighting style against them!\n";
            cout<<"  "<<dmg<<" dmg dealt using their own power!\n";
            break;
        }
        default: break;
    }
    ch.abilityUsed=true;
}

bool runCombat(Character &ch, Enemy en){

    // Reset per-combat state
    ch.defendBonus=0;
    ch.abilityUsed=false;
    ch.stealCooldown=0;

    printLine('-');
    ScaleFactor sf=getTier(ch.floor);
    cout<<"  ENCOUNTER: "<<en.name<<"  ["<<className(en.cls)<<"]";
    if(en.isBoss) cout<<"  *** BOSS ***";
    cout<<"\n";
    cout<<"  Tier: "<<sf.tierName
        <<"  |  HP:"<<en.hp
        <<"  ATK:"<<en.atk
        <<"  DEF:"<<en.def<<"\n";
    printLine('-');

    bool observedThisFight=false;
    bool observeBonus=false;

    while(ch.hp>0 && en.hp>0){

        // Tick distraction timer
        if(en.distracted){
            en.distractTurns--;
            if(en.distractTurns<=0){
                en.distracted=false;
                cout<<"  (The enemy snaps back to attention!)\n";
            }
        }

        // HUD
        cout<<"\n";
        printLine('-');
        cout<<"  Your HP: "<<ch.hp<<"/"<<ch.maxHp;
        if(ch.defendBonus>0) cout<<"  [DEF +"<<ch.defendBonus<<"]";
        if(ch.stealCooldown>0) cout<<"  [Steal CD:"<<ch.stealCooldown<<"]";
        cout<<"\n";
        cout<<"  "<<en.name<<" HP: "<<en.hp<<"/"<<en.maxHp;
        if(en.distracted) cout<<"  [DISTRACTED x"<<en.distractTurns<<"t]";
        if(en.stunned)    cout<<"  [STUNNED]";
        if(en.observed)   cout<<"  [STUDIED | weak:+"<<en.weaknessMod<<"]";
        cout<<"\n";
        printLine('-');

        // ---- Player rolls their dice before choosing an action ----
        // Flush leftover newline so the prompt always waits for a real keypress.
        cin.clear();
        cin.ignore(10000,'\n');
        cout<<"  Press ENTER to roll your dice...";
        cin.get();
        int diceRoll = rollDice(20);
        string diceLabel;
        if(diceRoll==20)      diceLabel="CRITICAL! Everything you do this turn is boosted!";
        else if(diceRoll>=15) diceLabel="Great roll! Strong bonus this turn.";
        else if(diceRoll>=10) diceLabel="Decent roll. Moderate bonus.";
        else if(diceRoll>=5)  diceLabel="Weak roll. Slight penalty.";
        else                  diceLabel="Terrible roll! Reduced effect this turn.";

        // Dice modifies the raw damage roll:
        //   20:    add 10 bonus flat + effects scale up
        //   15-19: add 5 bonus
        //   10-14: add 2 bonus
        //   5-9:   no bonus, no penalty
        //   1-4:   subtract 3 (but calcDamage min is still 1)
        int diceBonus=0;
        if(diceRoll==20)       diceBonus=10;
        else if(diceRoll>=15)  diceBonus=5;
        else if(diceRoll>=10)  diceBonus=2;
        else if(diceRoll<5)    diceBonus=-3;

        // A natural 20 also has a chance to add a free stun on any action
        bool natTwenty=(diceRoll==20);

        cout<<"  *** You rolled a " <<diceRoll<< " / 20 ***  " <<diceLabel<< "\n";
        printLine('-');

        // Menu
        cout<<"  [1] Attack      [2] Power Strike  [3] Defend\n";
        cout<<"  [4] Distract    [5] Assassinate   [6] Observe\n";
        cout<<"  [7] Taunt       [8] Steal         [9] Use Item\n";
        if(!ch.abilityUsed)
            cout<<"  [A] "<<classAbilityName(ch.cls)<<"\n";
        cout<<"  [0] Flee\n  > ";

        string input; cin>>input;
        int cmd=0;
        if(input=="a"||input=="A") cmd=99;
        else cmd=input[0]-'0';

        bool playerActed=true;

        if(cmd==1){
            // Normal Attack -- dice roll feeds directly into damage
            int dmg=calcDamage(ch.atk+getWeaponBonus(ch), en.def, diceRoll+diceBonus);
            if(observeBonus){ dmg+=en.weaknessMod; observeBonus=false;
                cout<<"  You exploit the weak point!\n"; }
            if(natTwenty){ en.stunned=true; cout<<"  NAT 20! "<<en.name<<" is STUNNED!\n"; }
            en.hp-=dmg;
            cout<<"  You hit "<<en.name<<" for "<<dmg<<" dmg!"
                <<"  (HP: "<<(en.hp<0?0:en.hp)<<"/"<<en.maxHp<<")\n";

        } else if(cmd==2){
            // Power Strike: 2x ATK + dice roll feeds in, costs 5 HP
            if(ch.hp<=5){ cout<<"  Not enough HP!\n"; playerActed=false; }
            else{
                ch.hp-=5;
                int dmg=calcDamage((ch.atk+getWeaponBonus(ch))*2, en.def, diceRoll+diceBonus);
                if(observeBonus){ dmg+=en.weaknessMod; observeBonus=false; }
                if(natTwenty){ en.stunned=true; cout<<"  NAT 20 POWER STRIKE! "<<en.name<<" is STUNNED!\n"; }
                en.hp-=dmg;
                cout<<"  POWER STRIKE! "<<dmg<<" dmg! (-5 HP stamina)\n";
            }

        } else if(cmd==3){
            // Defend: gain +8 def this round + dice-boosted counter
            ch.defendBonus=8;
            int dmg=calcDamage((ch.atk+getWeaponBonus(ch))/2, en.def, diceRoll+diceBonus);
            en.hp-=dmg;
            cout<<"  You raise your guard! (+8 DEF this round)\n";
            cout<<"  Counter-jab for "<<dmg<<" dmg.\n";

        } else if(cmd==4){
            // Distract -- high dice roll improves success chance
            int chance=en.isBoss?40:70;
            chance += diceBonus*2; // dice roll shifts odds
            if(diceRoll<=4) chance-=15; // bad roll makes it harder
            if(rollDice(100)<=chance){
                en.distracted=true;
                en.distractTurns=en.isBoss?1:rollDice(2)+1;
                if(natTwenty) en.distractTurns+=1; // nat 20 = longer distraction
                cout<<"  "<<DISTRACT_LINES[rand()%8]<<"\n";
                cout<<"  "<<en.name<<" is DISTRACTED for "<<en.distractTurns<<" turn(s)!\n";
                cout<<"  Use [5] Assassinate now!\n";
            } else {
                cout<<"  Distraction fails! "<<en.name<<" stays alert!\n";
                int eDmg=calcDamage(en.atk, ch.def+ch.defendBonus+getArmourBonus(ch), rollDice(12));
                ch.hp-=eDmg;
                cout<<"  "<<en.name<<" punishes you for "<<eDmg<<" dmg!\n";
                playerActed=false;
            }

        } else if(cmd==5){
            // Assassinate
            if(!en.distracted){
                cout<<"  Enemy is not distracted! Use [4] Distract first.\n";
                playerActed=false;
            } else if(en.isBoss){
                // Dice roll scales the multiplier (1.5x to 4x on nat20)
                float mult = 2.5f + (diceBonus * 0.1f);
                if(natTwenty) mult=4.0f;
                int dmg=(int)((ch.atk+getWeaponBonus(ch))*mult);
                en.hp-=dmg;
                en.distracted=false;
                cout<<"  "<<ASSASSINATE_LINES[rand()%6]<<"\n";
                cout<<"  Critical strike on distracted boss! "<<dmg<<" dmg!\n";
            } else {
                cout<<"  "<<ASSASSINATE_LINES[rand()%6]<<"\n";
                cout<<"  "<<en.name<<" is SLAIN INSTANTLY!\n";
                en.hp=0;
            }

        } else if(cmd==6){
            // Observe
            if(observedThisFight){
                cout<<"  Already studied. Next hit: +"<<en.weaknessMod<<" bonus dmg.\n";
                playerActed=false;
            } else {
                observedThisFight=true;
                observeBonus=true;
                en.observed=true;
                // High dice roll reveals a bigger weakness bonus
                if(natTwenty) en.weaknessMod += 5;
                else if(diceRoll>=15) en.weaknessMod += 2;
                cout<<"  You study "<<en.name<<" carefully...\n";
                cout<<"  Weak point found! Next attack +"<<en.weaknessMod<<" bonus dmg!\n";
            }

        } else if(cmd==7){
            // Taunt: enemy attacks, you counter through half their def
            // Dice roll improves your counter damage
            cout<<"  You taunt "<<en.name<<" viciously!\n";
            int eDmg=calcDamage(en.atk, ch.def+ch.defendBonus+getArmourBonus(ch), rollDice(12));
            ch.hp-=eDmg;
            cout<<"  "<<en.name<<" rages and deals "<<eDmg<<" dmg!\n";
            int cDmg=calcDamage(ch.atk+getWeaponBonus(ch), en.def/2, diceRoll+diceBonus);
            if(observeBonus){ cDmg+=en.weaknessMod; observeBonus=false; }
            en.hp-=cDmg;
            cout<<"  You counter through their rage for "<<cDmg<<" dmg!\n";
            // Nat 20 guarantees stun, otherwise need 9+ on d10
            if(natTwenty || rollDice(10)>=9){ en.stunned=true;
                cout<<"  "<<(natTwenty?"NAT 20! ":"CRITICAL TAUNT -- ")<<en.name<<" is STUNNED!\n"; }
            playerActed=false;

        } else if(cmd==8){
            // Steal -- dice roll shifts success chance
            if(ch.stealCooldown>0){
                cout<<"  Steal on cooldown ("<<ch.stealCooldown<<" turns).\n";
                playerActed=false;
            } else {
                int chance=en.distracted?80:40;
                if(en.isBoss) chance/=2;
                chance += diceBonus*3;
                if(rollDice(100)<=chance){
                    int stolen=rollDice(15)+5+diceBonus;
                    ch.gold+=stolen;
                    cout<<"  You pickpocket "<<en.name<<" for "<<stolen<<" gold!\n";
                    if(natTwenty || rollDice(10)>=8){
                        int idx=rand()%ITEM_DB_SIZE;
                        cout<<"  You also snatch: ";
                        addItem(ch,ITEM_DB[idx]);
                    }
                    ch.stealCooldown=3;
                } else {
                    cout<<"  Steal failed! "<<en.name<<" notices!\n";
                    int eDmg=calcDamage(en.atk, ch.def+ch.defendBonus+getArmourBonus(ch), rollDice(12));
                    ch.hp-=eDmg;
                    cout<<"  They retaliate for "<<eDmg<<" dmg!\n";
                    ch.stealCooldown=2;
                    playerActed=false;
                }
            }

        } else if(cmd==9){
            usePotion(ch);
            playerActed=false;

        } else if(cmd==99){
            if(ch.abilityUsed){
                cout<<"  Class ability already used this combat!\n";
                playerActed=false;
            } else {
                // Pass diceBonus to class ability by temporarily boosting ATK
                // (clean approach without rewriting doClassAbility signature)
                ch.atk += diceBonus;
                doClassAbility(ch,en);
                ch.atk -= diceBonus;
                if(natTwenty) cout<<"  NAT 20 bonus applied to your class ability!\n";
            }

        } else if(cmd==0){
            if(rollDice(10)>6){
                int fd=rollDice(8);
                ch.hp-=fd; if(ch.hp<0) ch.hp=0;
                cout<<"  You escape! (-"<<fd<<" HP)\n";
                return true;
            } else {
                cout<<"  Couldn't escape!\n";
                int eDmg=calcDamage(en.atk, ch.def+ch.defendBonus+getArmourBonus(ch), rollDice(12));
                ch.hp-=eDmg;
                cout<<"  "<<en.name<<" hits you for "<<eDmg<<" dmg!\n";
                playerActed=false;
            }

        } else {
            cout<<"  Invalid choice.\n";
            playerActed=false;
        }

        // Tick steal cooldown
        if(ch.stealCooldown>0) ch.stealCooldown--;

        if(en.hp<=0) break;

        // Enemy turn
        if(playerActed){
            if(en.stunned){
                cout<<"  "<<en.name<<" is stunned and cannot act!\n";
                en.stunned=false;
            } else if(!en.distracted || en.isBoss){
                // Distracted normal enemies skip their attack
                int eDef=ch.def+ch.defendBonus+getArmourBonus(ch);
                int eDmg=calcDamage(en.atk, eDef, rollDice(12));  // d12 = enemy hit variance
                ch.hp-=eDmg;
                cout<<"  "<<en.name<<" attacks for "<<eDmg<<" dmg!"
                    <<"  (Your HP: "<<(ch.hp<0?0:ch.hp)<<"/"<<ch.maxHp<<")\n";
            } else {
                cout<<"  "<<en.name<<" is distracted and ignores you!\n";
            }
        }

        ch.defendBonus=0; // reset defend bonus each round
    }

    if(ch.hp<=0){
        cout<<"\n  You have been defeated...\n";
        return false;
    }

    cout<<"\n  *** VICTORY! ***\n";
    int goldGain=en.isBoss ? rollDice(50)+(ch.floor*2)
                           : rollDice(20)+(ch.floor/2);
    ch.gold+=goldGain;
    cout<<"  Gained "<<goldGain<<" gold!\n";

    int dropChance=en.isBoss?90:45;
    if(rollDice(100)<=dropChance){
        int startIdx=0;
        if(ch.floor>=60) startIdx=35;
        else if(ch.floor>=40) startIdx=20;
        else if(ch.floor>=20) startIdx=10;
        int range=ITEM_DB_SIZE-startIdx;
        int idx=startIdx+(rand()%range);
        cout<<"  Enemy dropped: ";
        addItem(ch,ITEM_DB[idx]);
    }
    return true;
}

// ============================================================
//  NPC ENCOUNTER
// ============================================================
NPC makeNPC(int floor){
    NPC npc;
    npc.name=NPC_NAMES[rand()%20];
    npc.cls=(ClassType)(rand()%CLASS_COUNT);
    string greets[5]={
        "Greetings, traveller! Rough floors ahead.",
        "Careful, monsters grow stronger below.",
        "I sell my wares cheap. Take a look.",
        "Darkness lurks. Buy something to survive.",
        "A fellow wanderer! I have items for trade."};
    npc.greeting=greets[rand()%5];
    npc.shopSize=(rand()%3)+1;
    // deeper NPCs sell better items
    int startIdx=(floor>=60)?30:(floor>=40)?20:(floor>=20)?10:0;
    for(int i=0;i<npc.shopSize;i++){
        int range=ITEM_DB_SIZE-startIdx;
        npc.shop[i]=ITEM_DB[startIdx+(rand()%range)];
    }
    return npc;
}

void runNPC(Character &ch, int floor){
    NPC npc=makeNPC(floor);
    printLine('~');
    cout<<"  NPC ENCOUNTER!\n";
    printLine('~');
    cout<<"  You meet "<<npc.name<<" ["<<className(npc.cls)<<"]\n";
    if(ch.cls==SHAPESHIFTER && ch.disguised){
        cout<<"  (You appear as a "<<className(ch.disguiseAs)<<" to this NPC)\n";
        if(ch.disguiseAs==npc.cls)
            cout<<"  "<<npc.name<<": 'Ah, a fellow "<<className(npc.cls)<<"! 50% off!'\n";
    }
    cout<<"  "<<npc.name<<": \""<<npc.greeting<<"\"\n\n";
    cout<<"  [1] Browse shop  [2] Chat  [3] Leave\n  > ";
    int choice; cin>>choice;

    if(choice==1){
        printLine('-');
        cout<<"  SHOP  (Your gold: "<<ch.gold<<")\n";
        printLine('-');
        for(int i=0;i<npc.shopSize;i++){
            int price=npc.shop[i].value*5;
            if(ch.cls==SHAPESHIFTER && ch.disguised && ch.disguiseAs==npc.cls)
                price/=2;
            cout<<"  ["<<i+1<<"] "<<npc.shop[i].name<<"  ("<<price<<" gold)\n";
        }
        cout<<"  [0] Leave shop\n  > ";
        int buy; cin>>buy;
        if(buy>=1 && buy<=npc.shopSize){
            int price=npc.shop[buy-1].value*5;
            if(ch.cls==SHAPESHIFTER && ch.disguised && ch.disguiseAs==npc.cls)
                price/=2;
            if(ch.gold>=price){ ch.gold-=price; addItem(ch,npc.shop[buy-1]); }
            else cout<<"  Not enough gold.\n";
        }
    } else if(choice==2){
        string chats[4]={
            "tells you of ancient traps below.",
            "shares a rumour about a hidden vault.",
            "warns of an unusually powerful monster.",
            "gives a tip: bosses drop better loot at depth."};
        cout<<"  "<<npc.name<<" "<<chats[rand()%4]<<"\n";
    }
}

// ============================================================
//  SHAPESHIFTER
// ============================================================
void shapeshift(Character &ch){
    if(ch.cls!=SHAPESHIFTER){ cout<<"  Not a shapeshifter.\n"; return; }
    printLine();
    cout<<"  SHAPESHIFT\n  [1] Knight  [2] Paladin  [3] Mage\n";
    cout<<"  [4] Demon   [5] Skeleton [6] Revert\n  > ";
    int c; cin>>c;
    if(c>=1 && c<=5){ ch.disguised=true; ch.disguiseAs=(ClassType)(c-1);
        cout<<"  You transform into a "<<className(ch.disguiseAs)<<"!\n"; }
    else if(c==6){ ch.disguised=false; cout<<"  Reverted to true form.\n"; }
}

// ============================================================
//  PLAYER CREATION
// ============================================================
Character createPlayer(){
    Character ch;
    ch.invSize=0; ch.buffCount=0; ch.gold=50;
    ch.floor=1; ch.disguised=false;
    ch.defendBonus=0; ch.abilityUsed=false; ch.stealCooldown=0;

    printLine();
    cout<<"  DUNGEON CRAWLER v2 - CHARACTER CREATION\n";
    printLine();
    cout<<"  Enter your name: "; cin>>ch.name;

    cout<<"\n  Choose your class:\n";
    cout<<"  [1] Knight      - Balanced fighter\n";
    cout<<"  [2] Paladin     - High defence, holy power\n";
    cout<<"  [3] Mage        - High attack, low defence\n";
    cout<<"  [4] Demon       - Very high attack, risky\n";
    cout<<"  [5] Skeleton    - High defence, low HP\n";
    cout<<"  [6] Shapeshifter- Deception & shop discounts\n  > ";
    int c; cin>>c;
    if(c<1||c>6) c=1;
    ch.cls=(ClassType)(c-1);

    switch(ch.cls){
        case KNIGHT:       ch.maxHp=100; ch.atk=8;  ch.def=8;  break;
        case PALADIN:      ch.maxHp=90;  ch.atk=6;  ch.def=12; break;
        case MAGE:         ch.maxHp=70;  ch.atk=10; ch.def=4;  break;
        case DEMON:        ch.maxHp=80;  ch.atk=12; ch.def=3;  break;
        case SKELETON:     ch.maxHp=60;  ch.atk=8;  ch.def=14; break;
        case SHAPESHIFTER: ch.maxHp=85;  ch.atk=8;  ch.def=8;  break;
        default: ch.maxHp=100; ch.atk=8; ch.def=8; break;
    }
    ch.hp=ch.maxHp;
    // store base stats for save/load reference
    ch.baseAtk=ch.atk; ch.baseDef=ch.def; ch.baseMaxHp=ch.maxHp;

    cout<<"\n  Starting item: ";
    if(ch.cls==MAGE||ch.cls==DEMON) addItem(ch,ITEM_DB[5]);
    else addItem(ch,ITEM_DB[0]);

    cout<<"\n  Welcome, "<<ch.name<<" the "<<className(ch.cls)<<"!\n";
    cout<<"  HP:"<<ch.hp<<"  ATK:"<<ch.atk<<"  DEF:"<<ch.def<<"  GOLD:"<<ch.gold<<"\n";
    pause();
    return ch;
}

// ============================================================
//  STATUS DISPLAY
// ============================================================
void showStatus(const Character &ch){
    ScaleFactor sf=getTier(ch.floor);
    printLine();
    cout<<"  "<<ch.name<<" ["<<className(ch.cls)<<"]";
    if(ch.cls==SHAPESHIFTER && ch.disguised)
        cout<<" (disguised as "<<className(ch.disguiseAs)<<")";
    cout<<"\n";
    cout<<"  Floor: "<<ch.floor
        <<"  [Zone: "<<sf.tierName<<"]"
        <<"  |  HP: "<<ch.hp<<"/"<<ch.maxHp
        <<"  |  ATK: "<<ch.atk
        <<"  |  DEF: "<<ch.def
        <<"  |  GOLD: "<<ch.gold<<"\n";
    printLine();
}

// ============================================================
//  SAVE / LOAD SYSTEM
//  Format (plain text, one value per line):
//    Line 1:  player name
//    Line 2:  class (int)
//    Line 3:  hp
//    Line 4:  maxHp
//    Line 5:  atk
//    Line 6:  def
//    Line 7:  gold
//    Line 8:  floor
//    Line 9:  baseAtk
//    Line 10: baseDef
//    Line 11: baseMaxHp
//    Line 12: disguised (0/1)
//    Line 13: disguiseAs (int)
//    Line 14: invSize
//    Lines 15 to 14+invSize: itemName itemType itemValue itemActive
//    Next:    buffCount
//    Next buffCount lines: buffName buffDesc atkB defB hpB active
// ============================================================

// Scan the current folder for save_N.dat files
// Returns the number of saves found; fills slotExists[0..MAX_SAVE_SLOTS-1]
int scanSaveFiles(bool slotExists[]){
    int found=0;
    for(int i=1;i<=MAX_SAVE_SLOTS;i++){
        string fname=SAVE_PREFIX+to_string(i)+SAVE_EXT;
        ifstream f(fname.c_str());
        if(f.good()){ slotExists[i-1]=true; found++; }
        else slotExists[i-1]=false;
        f.close();
    }
    return found;
}

bool saveGame(const Character &ch, int slot){
    string fname=SAVE_PREFIX+to_string(slot)+SAVE_EXT;
    ofstream f(fname.c_str());
    if(!f.is_open()){ cout<<"  ERROR: Could not open "<<fname<<" for writing.\n"; return false; }

    f<<ch.name<<"\n";
    f<<(int)ch.cls<<"\n";
    f<<ch.hp<<"\n";
    f<<ch.maxHp<<"\n";
    f<<ch.atk<<"\n";
    f<<ch.def<<"\n";
    f<<ch.gold<<"\n";
    f<<ch.floor<<"\n";
    f<<ch.baseAtk<<"\n";
    f<<ch.baseDef<<"\n";
    f<<ch.baseMaxHp<<"\n";
    f<<(ch.disguised?1:0)<<"\n";
    f<<(int)ch.disguiseAs<<"\n";

    // Inventory
    f<<ch.invSize<<"\n";
    for(int i=0;i<ch.invSize;i++){
        // Replace spaces in names with underscores so reading is simple
        string n=ch.inventory[i].name;
        for(int j=0;j<(int)n.size();j++) if(n[j]==' ') n[j]='_';
        f<<n<<" "<<(int)ch.inventory[i].type<<" "
         <<ch.inventory[i].value<<" "<<(ch.inventory[i].active?1:0)<<"\n";
    }

    // Buffs
    f<<ch.buffCount<<"\n";
    for(int i=0;i<ch.buffCount;i++){
        string n=ch.buffs[i].name;
        string d=ch.buffs[i].description;
        for(int j=0;j<(int)n.size();j++) if(n[j]==' ') n[j]='_';
        for(int j=0;j<(int)d.size();j++) if(d[j]==' ') d[j]='_';
        f<<n<<" "<<d<<" "
         <<ch.buffs[i].atkBonus<<" "<<ch.buffs[i].defBonus<<" "
         <<ch.buffs[i].hpBonus<<" "<<(ch.buffs[i].active?1:0)<<"\n";
    }

    f.close();
    cout<<"  Game saved to slot "<<slot<<" ("<<fname<<")\n";
    return true;
}

bool loadGame(Character &ch, int slot){
    string fname=SAVE_PREFIX+to_string(slot)+SAVE_EXT;
    ifstream f(fname.c_str());
    if(!f.is_open()){ cout<<"  ERROR: Could not open "<<fname<<"\n"; return false; }

    int clsInt, disguisedInt, disguiseInt;
    f>>ch.name>>clsInt>>ch.hp>>ch.maxHp>>ch.atk>>ch.def
     >>ch.gold>>ch.floor>>ch.baseAtk>>ch.baseDef>>ch.baseMaxHp
     >>disguisedInt>>disguiseInt;
    ch.cls=(ClassType)clsInt;
    ch.disguised=(disguisedInt==1);
    ch.disguiseAs=(ClassType)disguiseInt;

    f>>ch.invSize;
    for(int i=0;i<ch.invSize;i++){
        string n; int t,v,a;
        f>>n>>t>>v>>a;
        for(int j=0;j<(int)n.size();j++) if(n[j]=='_') n[j]=' ';
        ch.inventory[i]={n,(ItemType)t,v,(a==1)};
    }

    f>>ch.buffCount;
    for(int i=0;i<ch.buffCount;i++){
        string n,d; int ab,db,hb,ac;
        f>>n>>d>>ab>>db>>hb>>ac;
        for(int j=0;j<(int)n.size();j++) if(n[j]=='_') n[j]=' ';
        for(int j=0;j<(int)d.size();j++) if(d[j]=='_') d[j]=' ';
        ch.buffs[i]={n,d,ab,db,hb,(ac==1)};
    }

    f.close();
    cout<<"  Loaded save from slot "<<slot<<" ("<<fname<<")\n";
    return true;
}

// Interactive save menu
void saveMenu(const Character &ch){
    printLine('=');
    cout<<"  SAVE GAME\n";
    printLine('=');
    bool slots[MAX_SAVE_SLOTS];
    scanSaveFiles(slots);
    for(int i=0;i<MAX_SAVE_SLOTS;i++){
        cout<<"  Slot "<<i+1<<": ";
        if(slots[i]){
            // Peek at name & floor from file
            string fname=SAVE_PREFIX+to_string(i+1)+SAVE_EXT;
            ifstream f(fname.c_str());
            string pname; int cls,hp,maxHp,atk,def,gold,fl;
            if(f>>pname>>cls>>hp>>maxHp>>atk>>def>>gold>>fl)
                cout<<"[USED] "<<pname<<" - Floor "<<fl;
            else cout<<"[CORRUPTED]";
            f.close();
        } else {
            cout<<"[EMPTY]";
        }
        cout<<"\n";
    }
    cout<<"  Choose slot (1-"<<MAX_SAVE_SLOTS<<") or 0 to cancel: ";
    int slot; cin>>slot;
    if(slot>=1 && slot<=MAX_SAVE_SLOTS)
        saveGame(ch, slot);
}

// Interactive load menu — returns true and fills ch if successful
bool loadMenu(Character &ch){
    printLine('=');
    cout<<"  LOAD GAME\n";
    printLine('=');
    bool slots[MAX_SAVE_SLOTS];
    int found=scanSaveFiles(slots);
    if(found==0){
        cout<<"  No save files found in this folder.\n";
        return false;
    }
    for(int i=0;i<MAX_SAVE_SLOTS;i++){
        cout<<"  Slot "<<i+1<<": ";
        if(slots[i]){
            string fname=SAVE_PREFIX+to_string(i+1)+SAVE_EXT;
            ifstream f(fname.c_str());
            string pname; int cls,hp,maxHp,atk,def,gold,fl;
            if(f>>pname>>cls>>hp>>maxHp>>atk>>def>>gold>>fl){
                ScaleFactor sf=getTier(fl);
                cout<<"[USED] "<<pname
                    <<" | Floor "<<fl
                    <<" | Zone: "<<sf.tierName
                    <<" | HP:"<<hp<<"/"<<maxHp
                    <<" | GOLD:"<<gold;
            } else cout<<"[CORRUPTED]";
            f.close();
        } else cout<<"[EMPTY]";
        cout<<"\n";
    }
    cout<<"  Choose slot (1-"<<MAX_SAVE_SLOTS<<") or 0 to cancel: ";
    int slot; cin>>slot;
    if(slot>=1 && slot<=MAX_SAVE_SLOTS && slots[slot-1]){
        return loadGame(ch, slot);
    }
    cout<<"  Load cancelled.\n";
    return false;
}

// ============================================================
//  MAIN MENU
// ============================================================
int main(){
    srand((unsigned)time(0));
    buildItemDB();

    cout<<"\n";
    printLine('#');
    cout<<"  ###   INFINITE DUNGEON CRAWLER v2   ###\n";
    cout<<"  ###   Now with Scaling & Save System ###\n";
    printLine('#');

    cout<<"\n  [1] New Game\n  [2] Load Game\n  > ";
    int choice; cin>>choice;

    Character player;
    bool loaded=false;

    if(choice==2){
        loaded=loadMenu(player);
        if(!loaded){
            cout<<"  Starting a new game instead.\n";
            pause();
            player=createPlayer();
        } else {
            cout<<"\n  Welcome back, "<<player.name<<"!\n";
            pause();
        }
    } else {
        player=createPlayer();
    }

    bool gameOver=false;
    bool quitByChoice=false;

    while(!gameOver){
        int fl=player.floor;

        cout<<"\n";
        showStatus(player);
        cout<<"  You descend to floor "<<fl<<"...\n";

        // Show difficulty warning on tier change
        if(fl==20||fl==40||fl==60||fl==80){
            ScaleFactor sf=getTier(fl);
            cout<<"\n  !!! DANGER ZONE ENTERED: "<<sf.tierName<<" !!!\n";
            cout<<"  Enemies are significantly stronger here. Be cautious.\n";
            pause();
        }
        cout<<"\n";

        // ---- Every 5 floors (not boss floors): Dice Roll ----
        if(fl%5==0 && fl%10!=0){
            diceRollEvent(player);
            if(player.hp<=0){ gameOver=true; break; }
            pause();
        }

        // ---- Every 10 floors: Boss ----
        if(fl%10==0){
            cout<<"  A crushing darkness fills the air...\n";
            pause();
            Enemy boss=makeEnemy(fl,true);
            if(!runCombat(player,boss)){ gameOver=true; break; }
            if(player.hp>0){
                cout<<"\n  Victory! A divine light fully heals you.\n";
                player.hp=player.maxHp;
            }
            pause();
        }
        // ---- Every 15 floors: NPC ----
        else if(fl%15==0){
            runNPC(player,fl);
            pause();
        }
        // ---- Normal floor: 1-3 encounters ----
        else {
            int numEnc=rollDice(3);
            cout<<"  You sense "<<numEnc<<" presence(s) in the dark...\n";
            pause();
            for(int e=0;e<numEnc && player.hp>0;e++){
                Enemy en=makeEnemy(fl,false);
                if(!runCombat(player,en)){ gameOver=true; break; }
                if(player.hp>0 && e<numEnc-1) pause();
            }
            if(gameOver) break;
        }

        if(player.hp<=0){ gameOver=true; break; }

        // ---- Floor menu ----
        bool nextFloor=false;
        while(!nextFloor && !gameOver){
            cout<<"\n";
            showStatus(player);
            cout<<"  [1] Descend   [2] Inventory  [3] Buffs\n";
            cout<<"  [4] Save Game [5] Quit to menu\n";
            if(player.cls==SHAPESHIFTER) cout<<"  [6] Shapeshift\n";
            cout<<"  > ";
            int cmd; cin>>cmd;
            if(cmd==1){
                nextFloor=true;
                player.floor++;
            } else if(cmd==2){
                showInventory(player);
                cout<<"  [1] Use item  [2] Drop item  [0] Back\n  > ";
                int ic; cin>>ic;
                if(ic==1) usePotion(player);
                else if(ic==2) dropItem(player);
            } else if(cmd==3){
                showBuffs(player);
                pause();
            } else if(cmd==4){
                saveMenu(player);
            } else if(cmd==5){
                cout<<"  Save before resting? (1=Yes / 2=No): ";
                int sq; cin>>sq;
                if(sq==1) saveMenu(player);
                gameOver=true;
                quitByChoice=true;
            } else if(cmd==6 && player.cls==SHAPESHIFTER){
                shapeshift(player);
            }
        }
    }

    // End screen
    cout<<"\n";
    if(quitByChoice){
        printLine('#');
        cout<<"  REST\n";
        cout<<"  "<<player.name<<" decided to rest on floor "<<player.floor<<".\n";
        cout<<"  Gold: "<<player.gold<<"  |  Buffs earned: "<<player.buffCount<<"\n";
        printLine('#');
        cout<<"\n  The dungeon will be waiting when you return.\n\n";
    } else {
        printLine('#');
        cout<<"  GAME OVER\n";
        cout<<"  "<<player.name<<" fell on floor "<<player.floor<<".\n";
        cout<<"  Gold: "<<player.gold<<"  |  Buffs earned: "<<player.buffCount<<"\n";
        printLine('#');
        cout<<"\n  Save final record? (1=Yes / 2=No): ";
        int fs; cin>>fs;
        if(fs==1) saveMenu(player);
        cout<<"\n  The darkness claims another soul.\n\n";
    }
    return 0;
}