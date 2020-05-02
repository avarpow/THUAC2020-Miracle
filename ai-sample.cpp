#include "ai_client.h"
#include "gameunit.h"
#include "card.hpp"
#include "calculator.h"

#include <fstream>
#include <stdlib.h>
#include <iostream>
#include <random>
#include <time.h>

using gameunit::Pos;
using gameunit::Unit;

using calculator::all_pos_in_map;
using calculator::cube_distance;
using calculator::cube_reachable;
using calculator::reachable;
using calculator::units_in_range;
using card::CARD_DICT;

using std::cerr;
using std::default_random_engine;
using std::endl;
using std::get;
using std::make_tuple;
using std::map;
using std::string;
using std::uniform_int_distribution;
using std::vector;
class AI : public AiClient
{
private:
    Pos miracle_pos;

    Pos enemy_pos;

    Pos target_barrack;

    Pos pos_1, pos_2, pos_3, pos_4, pos_5, pos_6, pos_7, pos_8, pos_9, pos_a, pos_b, pos_c, pos_d, pos_e, pos_p, pos_q, pos_r;

    int enemy_miracle_hp;

    int gj_summon_level, ms_summon_level, js_summon_level;
    //各个生物召唤出来的等级

    int enemy_camp;
    Unit archer1, archer2, js1, ms1, js2;

    string archer_str = "Archer", sworderman_str = "Swordsman", pristest_str = "Priest";
    enum attack_modes
    {
        ATTACK,
        DEFENSE,
        PROTECT
    } attack_mode;

    Pos posShift(Pos pos, string direct);

    Pos posShift_n(Pos pos, string direct, int num);

    void change_attack_mode();

    void game_init_pos();

    void set_summon_level(int level);

    void set_attack_mode();

    void attack_task(); //攻击指令

    void force_attack_task(); //发动勇猛，能打到则攻击

    void move_task(); //移动

    void set_summon_level_by_round(int round);

    void artifacts_task(); //使用神器

    void summon_task(); //召唤生物

public:
    //选择初始卡组
    void
    chooseCards(); //(根据初始阵营)选择初始卡组

    void play(); //玩家需要编写的ai操作函数

    void battle(); //处理生物的战斗

    void march(); //处理生物的移动

    int probobility(int p)
    {
        static default_random_engine g(static_cast<unsigned int>(time(nullptr)));
        int randnum = uniform_int_distribution<>(0, 100)(g);
        cerr << "random num" << p << endl;
        if (randnum <= p)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
};
void AI::set_attack_mode()
{
}
void AI::set_summon_level_by_round(int round)
{
    if (round == 0 || round == 1)
    {
        set_summon_level(1);
    }
    else if (round == 10 || round == 11)
    {
        set_summon_level(2);
    }
    else if (round == 14 || round == 15)
    {
        set_summon_level(3);
    }
    else if (round >= 24)
    {
        set_summon_level(4);
    }
}
void AI::set_summon_level(int level)
{
    if (level == 1)
    {
        gj_summon_level = 1;
        ms_summon_level = 2;
        js_summon_level = 1;
    }
    else if (level == 2)
    {
        gj_summon_level = 2;
        ms_summon_level = 2;
        js_summon_level = 2;
    }
    else if (level == 3)
    {
        gj_summon_level = 3;
        ms_summon_level = 2;
        js_summon_level = 2;
    }
    else if (level == 4)
    {
        gj_summon_level = 3;
        js_summon_level = 3;
        if (attack_mode == ATTACK)
        {
            ms_summon_level = 3;
        }
        else
        {
            ms_summon_level = 2;
        }
    }
}
Pos AI::posShift_n(Pos pos, string direct, int num)
{
    //偏移n次
    if (num <= 0)
        return pos;
    else
    {
        return posShift_n(posShift(pos, direct), direct, num - 1);
    }
}

void AI::chooseCards()
{
    // (根据初始阵营)选择初始卡组

    /*
     * artifacts和creatures可以修改
     * 【进阶】在选择卡牌时，就已经知道了自己的所在阵营和先后手，因此可以在此处根据先后手的不同设置不同的卡组和神器
     */
    //地狱火 弓箭 剑士 牧师
    my_artifacts = {"InfernoFlame"};
    my_creatures = {"Archer", "Swordsman", "Priest"};
    init();
}
void AI::game_init_pos()
{
    attack_mode = DEFENSE;
    //先确定自己的基地、对方的基地

    miracle_pos = map.miracles[my_camp].pos;
    enemy_pos = map.miracles[my_camp ^ 1].pos;
    enemy_camp = my_camp ^ 1;
    //设定目标驻扎点为最近的驻扎点

    target_barrack = map.barracks[0].pos;
    //确定离自己基地最近的驻扎点的位置

    pos_a = posShift_n(miracle_pos, "FF", 1);
    pos_b = posShift_n(miracle_pos, "IF", 1);
    pos_c = posShift_n(miracle_pos, "SF", 1);
    pos_d = posShift_n(pos_b, "IB", 1);
    pos_e = posShift_n(pos_c, "BB", 1);
    pos_p = posShift_n(target_barrack, "IF", 1);
    pos_q = posShift_n(target_barrack, "FF", 1);
    pos_r = posShift_n(target_barrack, "SF", 1);
    pos_1 = posShift_n(pos_d, "FF", 3);
    pos_6 = posShift_n(pos_1, "SF", 1);
    pos_5 = posShift_n(pos_6, "SF", 1);
    pos_2 = posShift_n(pos_6, "IF", 3);
    pos_3 = posShift_n(pos_2, "FF", 1);
    pos_7 = posShift_n(pos_5, "SF", 2);
    pos_4 = posShift_n(pos_7, "FF", 1);
    pos_9 = posShift_n(pos_c, "SF", 3);
}
void AI::attack_task()
{
}
void AI::move_task()
{
}
void AI::artifacts_task()
{
}
void AI::summon_task()
{
    //神器能用就用，选择覆盖单位数最多的地点
    if (players[my_camp].mana >= 6 && players[my_camp].artifact[0].state == "Ready")
    {
        auto pos_list = all_pos_in_map();
        vector<Pos> can_use_artifact_pos;
        for (auto i : pos_list)
        {
            if (canUseArtifact(players[my_camp].artifact[0], i, my_camp))
            {
                can_use_artifact_pos.push_back(i);
            }
        }
        auto best_pos = can_use_artifact_pos[0];
        int max_benefit = 0;
        for (auto pos : can_use_artifact_pos)
        {
            auto unit_list = units_in_range(pos, 2, map, enemy_camp);
            if (unit_list.size() > max_benefit)
            {
                best_pos = pos;
                max_benefit = unit_list.size();
            }
        }
        if (max_benefit >= 2)
        {
            use(players[my_camp].artifact[0].id, best_pos);
            cerr << "Use artifact at pos" << get<0>(best_pos) << " " << get<1>(best_pos) << " " << get<2>(best_pos) << " " << endl;
        }
    }
}
void AI::play()
{
    //玩家需要编写的ai操作函数

    /*
    本AI采用这样的策略：
    在首回合进行初期设置、在神迹优势路侧前方的出兵点召唤一个1星弓箭手
    接下来的每回合，首先尽可能使用神器，接着执行生物的战斗，然后对于没有进行战斗的生物，执行移动，最后进行召唤
    在费用较低时尽可能召唤星级为1的兵，优先度剑士>弓箭手>火山龙
    【进阶】可以对局面进行评估，优化神器的使用时机、调整每个生物行动的顺序、调整召唤的位置和生物种类、星级等
    */
    if (round == 0 || round == 1)
    {
        game_init_pos(); //初始化坐标
        set_summon_level(1);
        // 在正中心偏右召唤一个弓箭手，用来抢占驻扎点
        summon("Archer", 1, pos_c);
        archer1 = getUnitByPos(pos_c, false);
    }
    else if (round == 2 || round == 3)
    {
        move(archer1.id, pos_9);
        summon("Archer", 1, pos_b);
        archer2 = getUnitByPos(pos_b, false);
    }
    else if (round == 4 || round == 5)
    {
        move(archer1.id, target_barrack);
        move(archer2.id, pos_1);
    }
    else if (round == 6)
    {
        summon(sworderman_str, 1, pos_a);
        js1 = getUnitByPos(pos_a, false);
        move(archer2.id, pos_2);
        move(archer1.id, pos_4);
    }
    else if (round == 7)
    {
        summon(pristest_str, 2, pos_a);
        ms1 = getUnitByPos(pos_a, false);
        move(archer2.id, pos_2);
        move(archer1.id, pos_4);
    }
    else if (round == 8)
    {
        attack_task();
        if (js1.can_move)
        {
            move(js1.id, pos_6);
        }
        summon(pristest_str, 2, pos_a);
        ms1 = getUnitByPos(pos_a, false);
    }
    else if (round == 9)
    {
        attack_task();
        if (ms1.can_move)
        {
            move(ms1.id, pos_5);
        }
        summon(sworderman_str, 2, pos_p);
        js1 = getUnitByPos(pos_p, false);
    }
    else
    {
        set_summon_level_by_round(round);
        set_attack_mode();
        attack_task();
        move_task();
        artifacts_task();
        summon_task();

        //最后进行召唤
        //将所有本方出兵点按照到对方基地的距离排序，从近到远出兵
        auto summon_pos_list = getSummonPosByCamp(my_camp);
        sort(summon_pos_list.begin(), summon_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
            return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
        });
        vector<Pos> available_summon_pos_list;
        for (auto pos : summon_pos_list)
        {
            auto unit_on_pos_ground = getUnitByPos(pos, false);
            if (unit_on_pos_ground.id == -1)
                available_summon_pos_list.push_back(pos);
        }

        //统计各个生物的可用数量，在假设出兵点无限的情况下，按照1个剑士、1个弓箭手、1个火山龙的顺序召唤
        int mana = players[my_camp].mana;
        auto deck = players[my_camp].creature_capacity;
        ::map<string, int> available_count;
        for (const auto &card_unit : deck)
            available_count[card_unit.type] = card_unit.available_count;

        vector<string> summon_list;
        //剑士和弓箭手数量不足或者格子不足则召唤火山龙
        if ((available_summon_pos_list.size() == 1 || available_count["Swordsman"] + available_count["Archer"] < 2) &&
            mana >= CARD_DICT.at("VolcanoDragon")[1].cost && available_count["VolcanoDragon"] > 0)
        {
            summon_list.emplace_back("VolcanoDragon");
            mana -= CARD_DICT.at("VolcanoDragon")[1].cost;
        }

        bool suc = true;
        while (mana >= 2 && suc)
        {
            suc = false;
            if (available_count["Swordsman"] > 0 && mana >= CARD_DICT.at("Swordsman")[1].cost)
            {
                summon_list.emplace_back("Swordsman");
                mana -= CARD_DICT.at("Swordsman")[1].cost;
                available_count["Swordsman"] -= 1;
                suc = true;
            }
            if (available_count["Archer"] > 0 && mana >= CARD_DICT.at("Archer")[1].cost)
            {
                summon_list.emplace_back("Archer");
                mana -= CARD_DICT.at("Archer")[1].cost;
                available_count["Archer"] -= 1;
                suc = true;
            }
            if (available_count["VolcanoDragon"] > 0 && mana >= CARD_DICT.at("VolcanoDragon")[1].cost)
            {
                summon_list.emplace_back("VolcanoDragon");
                mana -= CARD_DICT.at("VolcanoDragon")[1].cost;
                available_count["VolcanoDragon"] -= 1;
                suc = true;
            }
        }

        int i = 0;
        for (auto pos : available_summon_pos_list)
        {
            if (i == summon_list.size())
                break;
            summon(summon_list[i], 1, pos);
            ++i;
        }
    }
    endRound();
}

Pos AI::posShift(Pos pos, string direct)
{
    /*
     * 对于给定位置，给出按照自己的视角（神迹在最下方）的某个方向移动一步后的位置
     * 本段代码可以自由取用
     * @param pos:  (x, y, z)
     * @param direct: 一个str，含2个字符，意义见注释
     * @return: 移动后的位置 (x', y', z')
     */
    transform(direct.begin(), direct.end(), direct.begin(), ::toupper);
    if (my_camp == 0)
    {
        if (direct == "FF") //正前方
            return make_tuple(get<0>(pos) + 1, get<1>(pos) - 1, get<2>(pos));
        else if (direct == "SF") //优势路前方（自身视角右侧为优势路）
            return make_tuple(get<0>(pos) + 1, get<1>(pos), get<2>(pos) - 1);
        else if (direct == "IF") //劣势路前方
            return make_tuple(get<0>(pos), get<1>(pos) + 1, get<2>(pos) - 1);
        else if (direct == "BB") //正后方
            return make_tuple(get<0>(pos) - 1, get<1>(pos) + 1, get<2>(pos));
        else if (direct == "SB") //优势路后方
            return make_tuple(get<0>(pos), get<1>(pos) - 1, get<2>(pos) + 1);
        else if (direct == "IB") //劣势路后方
            return make_tuple(get<0>(pos) - 1, get<1>(pos), get<2>(pos) + 1);
    }
    else
    {
        if (direct == "FF") //正前方
            return make_tuple(get<0>(pos) - 1, get<1>(pos) + 1, get<2>(pos));
        else if (direct == "SF") //优势路前方（自身视角右侧为优势路）
            return make_tuple(get<0>(pos) - 1, get<1>(pos), get<2>(pos) + 1);
        else if (direct == "IF") //劣势路前方
            return make_tuple(get<0>(pos), get<1>(pos) - 1, get<2>(pos) + 1);
        else if (direct == "BB") //正后方
            return make_tuple(get<0>(pos) + 1, get<1>(pos) - 1, get<2>(pos));
        else if (direct == "SB") //优势路后方
            return make_tuple(get<0>(pos), get<1>(pos) + 1, get<2>(pos) - 1);
        else if (direct == "IB") //劣势路后方
            return make_tuple(get<0>(pos) + 1, get<1>(pos), get<2>(pos) - 1);
    }
}

void AI::battle()
{
    //处理生物的战斗

    /*
     * 基本思路，行动顺序:
     * 火山龙：攻击高>低 （大AOE输出），随机攻击
     * 剑士：攻击低>高 打消耗，优先打攻击力低的
     * 弓箭手：攻击高>低 优先打不能反击的攻击力最高的，其次打能反击的攻击力最低的
     * 对单位的战斗完成后，对神迹进行输出
     * 【进阶】对战斗范围内敌方目标的价值进行评估，通过一些匹配算法决定最优的战斗方式
     * 例如占领着驻扎点的敌方生物具有极高的价值，优先摧毁可以使敌方下回合损失很多可用出兵点
     * 一些生物不攻击而移动，或完全不动，可能能带来更大的威慑力，而赢得更多优势
     */

    auto ally_list = getUnitsByCamp(my_camp);

    //自定义排列顺序
    auto cmp = [](const Unit &unit1, const Unit &unit2) {
        if (unit1.can_atk != unit2.can_atk) //首先要能动
            return unit2.can_atk < unit1.can_atk;
        else if (unit1.type != unit2.type)
        { //火山龙>剑士>弓箭手
            auto type_id_gen = [](const string &type_name) {
                if (type_name == "VolcanoDragon")
                    return 0;
                else if (type_name == "Swordsman")
                    return 1;
                else
                    return 2;
            };
            return (type_id_gen(unit1.type) < type_id_gen(unit2.type));
        }
        else if (unit1.type == "VolcanoDragon" or unit1.type == "Archer")
            return unit2.atk < unit1.atk;
        else
            return unit1.atk < unit2.atk;
    };
    //按顺序排列好单位，依次攻击
    sort(ally_list.begin(), ally_list.end(), cmp);
    for (const auto &ally : ally_list)
    {
        if (!ally.can_atk)
            break;
        auto enemy_list = getUnitsByCamp(my_camp ^ 1);
        vector<Unit> target_list;
        for (const auto &enemy : enemy_list)
            if (AiClient::canAttack(ally, enemy))
                target_list.push_back(enemy);
        if (target_list.empty())
            continue;
        if (ally.type == "VolcanoDragon")
        {
            default_random_engine g(static_cast<unsigned int>(time(nullptr)));
            int tar = uniform_int_distribution<>(0, target_list.size() - 1)(g);
            attack(ally.id, target_list[tar].id);
        }
        else if (ally.type == "Swordsman")
        {
            nth_element(enemy_list.begin(), enemy_list.begin(), enemy_list.end(),
                        [](const Unit &_enemy1, const Unit &_enemy2) { return _enemy1.atk < _enemy2.atk; });
            attack(ally.id, target_list[0].id);
        }
        else if (ally.type == "Archer")
        {
            sort(enemy_list.begin(), enemy_list.end(),
                 [](const Unit &_enemy1, const Unit &_enemy2) { return _enemy1.atk > _enemy2.atk; });
            bool suc = false;
            for (const auto &enemy : target_list)
                if (!canAttack(enemy, ally))
                {
                    attack(ally.id, enemy.id);
                    suc = true;
                    break;
                }
            if (suc)
                continue;
            nth_element(enemy_list.begin(), enemy_list.begin(), enemy_list.end(),
                        [](const Unit &_enemy1, const Unit &_enemy2) { return _enemy1.atk < _enemy2.atk; });
            attack(ally.id, target_list[0].id);
        }
    }
    //最后攻击神迹
    ally_list = getUnitsByCamp(my_camp);
    sort(ally_list.begin(), ally_list.end(), cmp);
    for (auto ally : ally_list)
    {
        if (!ally.can_atk)
            break;
        int dis = cube_distance(ally.pos, enemy_pos);
        if (ally.atk_range[0] <= dis && dis <= ally.atk_range[1])
            attack(ally.id, my_camp ^ 1);
    }
}

void AI::march()
{
    //处理生物的移动

    /*
     * 先动所有剑士，尽可能向敌方神迹移动
     * 若目标驻扎点上没有地面单位，则让弓箭手向目标驻扎点移动，否则尽可能向敌方神迹移动
     * 然后若目标驻扎点上没有地面单位，则让火山之龙向目标驻扎点移动，否则尽可能向敌方神迹移动
     * 【进阶】一味向敌方神迹移动并不一定是个好主意
     * 在移动的时候可以考虑一下避开敌方生物攻击范围实现、为己方强力生物让路、堵住敌方出兵点等策略
     * 如果采用其他生物组合，可以考虑抢占更多驻扎点
     */
    auto ally_list = getUnitsByCamp(my_camp);
    sort(ally_list.begin(), ally_list.end(), [](const Unit &_unit1, const Unit &_unit2) {
        auto type_id_gen = [](const string &type_name) {
            if (type_name == "Swordsman")
                return 0;
            else if (type_name == "Archer")
                return 1;
            else
                return 2;
        };
        return type_id_gen(_unit1.type) < type_id_gen(_unit2.type);
    });
    for (const auto &ally : ally_list)
    {
        if (!ally.can_move)
            continue;
        if (ally.type == "Swordsman")
        {
            //获取所有可到达的位置
            auto reach_pos_with_dis = reachable(ally, map);
            //压平
            vector<Pos> reach_pos_list;
            for (const auto &reach_pos : reach_pos_with_dis)
            {
                for (auto pos : reach_pos)
                    reach_pos_list.push_back(pos);
            }
            if (reach_pos_list.empty())
                continue;
            //优先走到距离敌方神迹更近的位置
            nth_element(reach_pos_list.begin(), reach_pos_list.begin(), reach_pos_list.end(),
                        [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                        });
            move(ally.id, reach_pos_list[0]);
        }
        else
        {
            //如果已经在兵营就不动了
            bool on_barrack = false;
            for (const auto &barrack : map.barracks)
                if (ally.pos == barrack.pos)
                {
                    on_barrack = true;
                    break;
                }
            if (on_barrack)
                continue;

            //获取所有可到达的位置
            auto reach_pos_with_dis = reachable(ally, map);
            //压平
            vector<Pos> reach_pos_list;
            for (const auto &reach_pos : reach_pos_with_dis)
            {
                for (auto pos : reach_pos)
                    reach_pos_list.push_back(pos);
            }
            if (reach_pos_list.empty())
                continue;

            //优先走到未被占领的兵营，否则走到
            if (getUnitByPos(target_barrack, false).id == -1)
            {
                nth_element(reach_pos_list.begin(), reach_pos_list.begin(), reach_pos_list.end(),
                            [this](Pos _pos1, Pos _pos2) {
                                return cube_distance(_pos1, target_barrack) < cube_distance(_pos2, target_barrack);
                            });
                move(ally.id, reach_pos_list[0]);
            }
            else
            {
                nth_element(reach_pos_list.begin(), reach_pos_list.begin(), reach_pos_list.end(),
                            [this](Pos _pos1, Pos _pos2) {
                                return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                            });
                move(ally.id, reach_pos_list[0]);
            }
        }
    }
}

int main()
{
    std::ifstream datafile("Data.json");
    json all_data;
    datafile >> all_data;
    card::get_data_from_json(all_data);

    AI player_ai;
    player_ai.chooseCards();
    while (true)
    {
        player_ai.updateGameInfo();
        player_ai.play();
    }
    return 0;
}
