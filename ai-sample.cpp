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
struct pos_with_value
{
    Pos pos;
    int value1;
    int value2;
};

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

    string archer_str = "Archer", sworderman_str = "Swordsman", pristest_str = "Priest", inferno_str = "Inferno";
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

    bool near_my_miracle(Unit unit); //检测生物距离我方基地在7格以内

    int getvalue(Unit a);

    int cmp(Unit a, Unit b);

    bool canAttackMiracle(Unit a);

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
bool AI::canAttackMiracle(Unit a)
{
    //能否达到地方基地
    int dis = cube_distance(a.pos, enemy_pos);
    if (dis >= a.atk_range[0] && dis <= a.atk_range[1])
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool AI::near_my_miracle(Unit unit)
{
    if (cube_distance(unit.pos, miracle_pos) <= 6)
    {
        return true;
    }
    else
    {
        return false;
    }
}
void AI::set_attack_mode()
{
    int protect_mode_flag = 0;
    int enemy_population; //敌方人口数量
    int my_population;    //我方人口数量
    auto enemy_allay_list = getUnitsByCamp(enemy_camp);
    auto my_allay_list = getUnitsByCamp(my_camp);
    enemy_population = enemy_allay_list.size();
    my_population = my_allay_list.size();
    for (auto allay : enemy_allay_list)
    {
        if (cube_distance(allay.pos, miracle_pos) <= 3)
        {
            protect_mode_flag = 1;
            break;
        }
    }
    if (protect_mode_flag == 1)
    { //protect模式，有敌方单位在3格以内
        attack_mode = PROTECT;
        cerr << "PROTECT mode" << endl;
        return;
    }
    if (my_population >= enemy_population)
    {
        attack_mode = ATTACK;
        cerr << "ATTACK mode" << endl;
        return;
    }
    else
    {
        attack_mode = DEFENSE;
        cerr << "DENFENSE mode" << endl;
        return;
    }
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
    enemy_miracle_hp = 30;
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
int AI::getvalue(Unit a)
{
    if (a.type == sworderman_str)
    {
        return 1;
    }
    else if (a.type == inferno_str)
    {
        return 2;
    }
    else if (a.type == archer_str)
    {
        return 3;
    }
    else
        return 4;
}
int AI::cmp(Unit a, Unit b)
{
    if (getvalue(a) != getvalue(b))
    {
        return getvalue(a) < getvalue(b);
    }
    else
    {
        return cube_distance(miracle_pos, a.pos) < cube_distance(miracle_pos, b.pos);
    }
}
void AI::attack_task() //仅仅考虑单个单位致命，不考虑多个单位联合致命
{
    auto my_allay_list = getUnitsByCamp(my_camp);
    sort(my_allay_list.begin(), my_allay_list.end(), cmp);
    //按照剑士，地狱火，弓箭手，牧师排序，相同类型距离基地近的排在前面
    auto enemy_allay_list = getUnitsByCamp(enemy_camp);
    int shield_flag = 0;
    Unit shield_unit;
    //检测是否能杀死圣盾怪
    //1检测是否圣盾存在
    for (auto enemy : enemy_allay_list)
    {
        if (enemy.holy_shield == true)
        {
            shield_flag = 1;
            shield_unit = enemy;
            break;
        }
    }
    //2检测是否能杀死圣盾
    vector<Unit> atk_sield;
    int shield_damage = 0;
    for (auto allay : my_allay_list)
    {
        if (allay.can_atk == true && canAttack(allay, shield_unit))
        {
            shield_damage += allay.atk;
            atk_sield.push_back(allay);
        }
    }
    if (shield_damage >= shield_unit.hp)
    {
        for (auto allay : atk_sield)
        {
            if (allay.can_atk == true && canAttack(allay, shield_unit) && shield_unit.hp > 0)
            {
                attack(allay.id, shield_unit.id);
                cerr << "6 unit id " << allay.id << " attack unit id" << shield_unit.id << endl;
            }
        }
    }
    //=========
    for (auto allay : my_allay_list)
    {
        //第五优先级：对方在基地三格以内，我不是牧师
        if (allay.can_atk == true && allay.type != pristest_str)
        {
            int target_id = -1;
            for (auto enemy : enemy_allay_list)
            {
                if (canAttack(allay, enemy) && (cube_distance(miracle_pos, enemy.pos) <= 3) && enemy.hp > 0)
                {
                    target_id = enemy.id;
                }
            }
            if (target_id != -1)
            {
                Unit target = getUnitById(target_id);
                //检测目标hp大于0
                if (target.hp > 0)
                {
                    attack(allay.id, target_id);
                    cerr << "5 unit id " << allay.id << " attack unit id" << target_id << endl;
                }
            }
        }
    }
    for (auto allay : my_allay_list)
    {
        //第四优先级，对方是牧师，我可以杀死对方
        if (allay.can_atk)
        {
            int target_id = -1;
            for (auto enemy : enemy_allay_list)
            {
                if (enemy.type == pristest_str && canAttack(allay, enemy) && allay.atk >= enemy.hp && enemy.hp > 0)
                {
                    target_id = enemy.id;
                }
            }
            if (target_id != -1)
            {
                Unit target = getUnitById(target_id);
                if (target.hp > 0)
                {
                    attack(allay.id, target_id);
                    cerr << "4 unit id " << allay.id << " attack unit id" << target_id << endl;
                }
            }
        }
    }
    for (auto allay : my_allay_list)
    {
        //第三优先级，可以杀死对方，对方不能杀死我
        if (allay.can_atk)
        {
            int target_id = -1;
            for (auto enemy : enemy_allay_list)
            {
                //能杀死
                if (canAttack(allay, enemy) && allay.atk >= enemy.hp)
                {
                    //对方打不死我或者对方打不到我
                    if (canAttack(enemy, allay) == false || canAttack(enemy, allay) && enemy.atk < allay.hp && enemy.hp > 0)
                    {
                        target_id = enemy.id;
                    }
                }
            }
            if (target_id != -1)
            {
                Unit target = getUnitById(target_id);
                if (target.hp > 0)
                {
                    attack(allay.id, target_id);
                    cerr << "3 unit id " << allay.id << " attack unit id" << target_id << endl;
                }
            }
        }
    }
    for (auto allay : my_allay_list)
    {
        //第二优先级 可以攻击基地
        if (allay.can_atk)
        {
            if (canAttackMiracle(allay))
            {
                attack(allay.id, enemy_camp);
                enemy_miracle_hp -= allay.atk;
                cerr << "2 attack miracle  left hp:" << enemy_miracle_hp << endl;
            }
        }
    }
    for (auto allay : my_allay_list)
    {
        //第一优先级 可以攻击敌方单位但是自身不会致命
        if (allay.can_atk)
        {
            int target_id = -1;
            for (auto enemy : enemy_allay_list)
            {
                if (canAttack(allay, enemy))
                {
                    //对方打不死我或者对方打不到我
                    if (canAttack(enemy, allay) == false || canAttack(enemy, allay) && enemy.atk < allay.hp && enemy.hp > 0)
                    {
                        target_id = enemy.id;
                    }
                }
            }
            if (target_id != -1)
            {
                Unit target = getUnitById(target_id);
                if (target.hp > 0)
                {
                    attack(allay.id, target_id);
                    cerr << "1 unit id " << allay.id << " attack unit id" << target_id << endl;
                }
            }
        }
    }
    for (auto allay : my_allay_list)
    {
        //第零优先级 可以换掉对面且对面在我方半区
        if (allay.can_atk && allay.type != pristest_str)
        {
            int target_id = -1;
            for (auto enemy : enemy_allay_list)
            {
                //对方在我方半区，不管我方单位死活，我的单位可以杀死对方，
                if (near_my_miracle(enemy) && canAttack(allay, enemy) && enemy.hp > 0 && allay.atk > enemy.hp)
                {
                    target_id = enemy.id;
                }
            }
            if (target_id != -1)
            {
                Unit target = getUnitById(target_id);
                if (target.hp > 0)
                {
                    attack(allay.id, target_id);
                    cerr << "0 unit id " << allay.id << " attack unit id" << target_id << endl;
                }
            }
        }
    }
    //攻击结束
}
void AI::move_task()
{
    auto my_allay_list = getUnitsByCamp(my_camp);
    sort(my_allay_list.begin(), my_allay_list.end(), cmp);
    //按照剑士，地狱火，弓箭手，牧师排序，相同类型距离基地近的排在前面
    auto enemy_allay_list = getUnitsByCamp(enemy_camp);
    if (attack_mode == ATTACK)
    {
        for (auto allay : my_allay_list)
        {
            //准备每个距离可以到达的点和所有可达的点
            auto reach_pos_with_dis = reachable(allay, map);
            vector<Pos> reach_pos_list;
            for (const auto &reach_pos : reach_pos_with_dis)
            {
                for (auto pos : reach_pos)
                    reach_pos_list.push_back(pos);
            }
            if (allay.can_move && allay.type != pristest_str)
            {
                //第一优先级 占领驻扎点
                for (auto pos : reach_pos_list)
                {
                    int barrack_status = checkBarrack(pos);
                    if (barrack_status == -1 || barrack_status == enemy_camp)
                    {
                        if (getUnitByPos(pos, false).id == -1)
                        { //该驻扎点可以占领
                            cerr << "move id " << allay.id << " to " << get<0>(pos) << " " << get<1>(pos) << " " << get<2>(pos) << " "
                                 << endl;
                            move(allay.id, pos);
                        }
                    }
                }
            }
            if (allay.can_move)
            {
                //攻击状态第二优先级
                if (allay.type == sworderman_str || allay.type == inferno_str)
                {
                    //剑士 或者地狱火 50%朝向能打到敌方最多的点移动 50%朝向最靠近地方的非致命位置最靠近对方基地的位置移动
                    int hit = probobility(50); //检测是否被概率命中，概率为百分比
                    if (hit == 1)
                    {
                        vector<pos_with_value> max_enemy_attack_pos;
                        for (auto pos : reach_pos_list)
                        {
                            int enemy_conut = 0; //如果走到该pos能打到敌方多少单位
                            for (auto enemy : enemy_allay_list)
                            {
                                if (enemy.flying == true && allay.atk_flying == true)
                                {
                                    int dis = cube_distance(pos, enemy.pos);
                                    if (dis >= allay.atk_range[0] && dis <= allay.atk_range[1])
                                    {
                                        enemy_conut++;
                                    }
                                }
                                else
                                {
                                    int dis = cube_distance(pos, enemy.pos);
                                    if (dis >= allay.atk_range[0] && dis <= allay.atk_range[1])
                                    {
                                        enemy_conut++;
                                    }
                                }
                            }
                            pos_with_value temp;
                            temp.pos = pos;
                            temp.value1 = enemy_conut;
                            temp.value2 = 0;
                            max_enemy_attack_pos.push_back(temp);
                        }
                        sort(max_enemy_attack_pos.begin(), max_enemy_attack_pos.end(), [&](pos_with_value a, pos_with_value b) {
                            return a.value1 > b.value1;
                        });
                        if (!max_enemy_attack_pos.empty() && max_enemy_attack_pos.front().value1 > 0)
                        {
                            cerr << "move id " << allay.id << " to " << get<0>(max_enemy_attack_pos.front().pos) << " " << get<1>(max_enemy_attack_pos.front().pos) << " " << get<2>(max_enemy_attack_pos.front().pos) << " "
                                 << endl;
                            move(allay.id, max_enemy_attack_pos.front().pos);
                        }
                    }
                    if (allay.can_move)
                    { //上面没有移动所以还能移动
                        //朝向最靠近地方的非致命位置最靠近对方基地的位置移动
                        vector<pos_with_value> nearest_enemy_mircle_pos;
                        for (auto pos : reach_pos_list)
                        {
                            int max_damage = 0;
                            for (auto enemy : enemy_allay_list)
                            {
                                int dis = cube_distance(pos, enemy.pos);
                                if (dis >= enemy.atk_range[0] && dis <= enemy.atk_range[1])
                                {
                                    if (enemy.atk > max_damage)
                                    {
                                        max_damage = enemy.atk;
                                    }
                                }
                            }
                            if (max_damage < allay.hp)
                            {
                                pos_with_value temp;
                                temp.pos = pos;
                                temp.value1 = cube_distance(enemy_pos, pos);
                                temp.value2 = 0;
                                nearest_enemy_mircle_pos.push_back(temp);
                            }
                        }
                        sort(nearest_enemy_mircle_pos.begin(), nearest_enemy_mircle_pos.end(), [&](pos_with_value a, pos_with_value b) {
                            return a.value1 < b.value1;
                        });
                        if (!nearest_enemy_mircle_pos.empty())
                        {
                            cerr << "move id " << allay.id << " to " << get<0>(nearest_enemy_mircle_pos.front().pos) << " " << get<1>(nearest_enemy_mircle_pos.front().pos) << " " << get<2>(nearest_enemy_mircle_pos.front().pos) << " "
                                 << endl;
                            move(allay.id, nearest_enemy_mircle_pos.front().pos);
                        }
                    }
                    //=====攻击模式 剑士 地狱火 完===================================
                }
                else if (allay.type == archer_str)
                {
                    //弓箭手 朝向距离对方基地最近的非致命位置移动
                    if (allay.can_move)
                    { //上面没有移动所以还能移动
                        //朝向最靠近地方的非致命位置最靠近对方基地的位置移动
                        vector<pos_with_value> nearest_enemy_mircle_pos;
                        for (auto pos : reach_pos_list)
                        {
                            int max_damage = 0;
                            for (auto enemy : enemy_allay_list)
                            {
                                int dis = cube_distance(pos, enemy.pos);
                                if (dis >= enemy.atk_range[0] && dis <= enemy.atk_range[1])
                                {
                                    if (enemy.atk > max_damage)
                                    {
                                        max_damage = enemy.atk;
                                    }
                                }
                            }
                            if (max_damage < allay.hp)
                            {
                                //弓箭手特殊距离计算 使得目标点能打到基地
                                pos_with_value temp;
                                temp.pos = pos;
                                int enemy_pos_dis = cube_distance(enemy_pos, pos);
                                if (enemy_pos_dis <= 3)
                                {
                                    enemy_pos_dis = 3 - enemy_pos_dis;
                                }
                                else if (enemy_pos_dis >= 4)
                                {
                                    enemy_pos_dis = enemy_pos_dis - 4;
                                }
                                temp.value1 = enemy_pos_dis;
                                temp.value2 = 0;
                                nearest_enemy_mircle_pos.push_back(temp);
                            }
                        }
                        sort(nearest_enemy_mircle_pos.begin(), nearest_enemy_mircle_pos.end(), [&](pos_with_value a, pos_with_value b) {
                            return a.value1 < b.value1;
                        });
                        if (!nearest_enemy_mircle_pos.empty())
                        {
                            cerr << "move id " << allay.id << " to " << get<0>(nearest_enemy_mircle_pos.front().pos) << " " << get<1>(nearest_enemy_mircle_pos.front().pos) << " " << get<2>(nearest_enemy_mircle_pos.front().pos) << " "
                                 << endl;
                            move(allay.id, nearest_enemy_mircle_pos.front().pos);
                        }
                    }
                }
                else if (allay.type == pristest_str)
                {
                    if (allay.can_move)
                    {
                        //朝向能覆盖最多己方单位的最靠近己方神迹的点行动
                        vector<pos_with_value> most_cover_my_unit_pos;
                        for (auto pos : reach_pos_list)
                        {
                            auto unit_list = units_in_range(pos, 3, map, my_camp);
                            int cover_num = unit_list.size();
                            pos_with_value temp;
                            temp.pos = pos;
                            temp.value1 = cover_num;
                            temp.value2 = cube_distance(pos, miracle_pos);
                            most_cover_my_unit_pos.push_back(temp);
                        }
                        sort(most_cover_my_unit_pos.begin(), most_cover_my_unit_pos.end(), [&](pos_with_value a, pos_with_value b) {
                            if (a.value1 != b.value1)
                            {
                                return a.value1 > b.value1;
                            }
                            else
                            {
                                return a.value2 < b.value2;
                            }
                        });
                        if (!most_cover_my_unit_pos.empty())
                        {
                            cerr << "move id " << allay.id << " to " << get<0>(most_cover_my_unit_pos.front().pos) << " " << get<1>(most_cover_my_unit_pos.front().pos) << " " << get<2>(most_cover_my_unit_pos.front().pos) << " "
                                 << endl;
                            move(allay.id, most_cover_my_unit_pos.front().pos);
                        }
                    }
                }
                //攻击模式 牧师移动 完
            }
        }
        //攻击模式完
    }
    else if (attack_mode == DEFENSE)
    {
        for (auto allay : my_allay_list)
        {
            //准备每个距离可以到达的点和所有可达的点
            auto reach_pos_with_dis = reachable(allay, map);
            vector<Pos> reach_pos_list;
            for (const auto &reach_pos : reach_pos_with_dis)
            {
                for (auto pos : reach_pos)
                    reach_pos_list.push_back(pos);
            }
            if (allay.can_move && allay.type != pristest_str)
            {
                //第一优先级 占领驻扎点
                for (auto pos : reach_pos_list)
                {
                    int barrack_status = checkBarrack(pos);
                    if (barrack_status == -1 || barrack_status == enemy_camp)
                    {
                        if (getUnitByPos(pos, false).id == -1)
                        { //该驻扎点可以占领
                            cerr << "move id " << allay.id << " to " << get<0>(pos) << " " << get<1>(pos) << " " << get<2>(pos) << " "
                                 << endl;
                            move(allay.id, pos);
                        }
                    }
                }
            }
            if (allay.can_move)
            {
                //攻击状态第二优先级
                if (allay.type == sworderman_str || allay.type == inferno_str)
                {
                    //剑士 或者地狱火 朝向不超过中线，能打到敌方单位最多的点移动 否则朝向最靠近位置7 8的位置移动
                    vector<pos_with_value> max_enemy_attack_pos;
                    for (auto pos : reach_pos_list)
                    {
                        int enemy_conut = 0; //如果走到该pos能打到敌方多少单位
                        for (auto enemy : enemy_allay_list)
                        {
                            if (enemy.flying == true && allay.atk_flying == true)
                            {
                                int dis = cube_distance(pos, enemy.pos);
                                if (dis >= allay.atk_range[0] && dis <= allay.atk_range[1])
                                {
                                    enemy_conut++;
                                }
                            }
                            else
                            {
                                int dis = cube_distance(pos, enemy.pos);
                                if (dis >= allay.atk_range[0] && dis <= allay.atk_range[1])
                                {
                                    enemy_conut++;
                                }
                            }
                        }
                        pos_with_value temp;
                        temp.pos = pos;
                        temp.value1 = enemy_conut;
                        temp.value2 = 0;
                        if (cube_distance(pos, miracle_pos) <= 6)
                        {
                            max_enemy_attack_pos.push_back(temp);
                        }
                    }
                    sort(max_enemy_attack_pos.begin(), max_enemy_attack_pos.end(), [&](pos_with_value a, pos_with_value b) {
                        return a.value1 > b.value1;
                    });
                    if (!max_enemy_attack_pos.empty() && max_enemy_attack_pos.front().value1 > 0)
                    {
                        cerr << "move id " << allay.id << " to " << get<0>(max_enemy_attack_pos.front().pos) << " " << get<1>(max_enemy_attack_pos.front().pos) << " " << get<2>(max_enemy_attack_pos.front().pos) << " "
                             << endl;
                        move(allay.id, max_enemy_attack_pos.front().pos);
                    }
                    if (allay.can_move)
                    {
                        //否则朝向最靠近位置7 8的位置移动
                        vector<pos_with_value> max_near78_attack_pos;
                        for (auto pos : reach_pos_list)
                        {
                            int dis7 = cube_distance(pos_7, pos);
                            int dis8 = cube_distance(pos_8, pos);
                            int min_dis = std::min(dis7, dis8);
                            pos_with_value temp;
                            temp.pos = pos;
                            temp.value1 = min_dis;
                            temp.value2 = 0;
                            max_enemy_attack_pos.push_back(temp);
                        }
                        sort(max_enemy_attack_pos.begin(), max_enemy_attack_pos.end(), [&](pos_with_value a, pos_with_value b) {
                            return a.value1 < b.value1;
                        });
                        if (!max_enemy_attack_pos.empty())
                        {
                            cerr << "move id " << allay.id << " to " << get<0>(max_enemy_attack_pos.front().pos) << " " << get<1>(max_enemy_attack_pos.front().pos) << " " << get<2>(max_enemy_attack_pos.front().pos) << " "
                                 << endl;
                            move(allay.id, max_enemy_attack_pos.front().pos);
                        }
                    }
                }
                else if (allay.type == archer_str)
                {
                    //弓箭手 朝向距离最靠近位置5的非致命位置移动
                    if (allay.can_move)
                    {
                        //朝向距离最靠近位置5的非致命位置移动
                        vector<pos_with_value> nearest_pos5_mircle_pos;
                        for (auto pos : reach_pos_list)
                        {
                            int max_damage = 0;
                            for (auto enemy : enemy_allay_list)
                            {
                                int dis = cube_distance(pos, enemy.pos);
                                if (dis >= enemy.atk_range[0] && dis <= enemy.atk_range[1])
                                {
                                    if (enemy.atk > max_damage)
                                    {
                                        max_damage = enemy.atk;
                                    }
                                }
                            }
                            if (max_damage < allay.hp)
                            {
                                pos_with_value temp;
                                temp.pos = pos;
                                int pos5_pos_dis = cube_distance(pos_5, pos);
                                temp.value1 = pos5_pos_dis;
                                temp.value2 = 0;
                                nearest_pos5_mircle_pos.push_back(temp);
                            }
                        }
                        sort(nearest_pos5_mircle_pos.begin(), nearest_pos5_mircle_pos.end(), [&](pos_with_value a, pos_with_value b) {
                            return a.value1 < b.value1;
                        });
                        if (!nearest_pos5_mircle_pos.empty())
                        {
                            cerr << "move id " << allay.id << " to " << get<0>(nearest_pos5_mircle_pos.front().pos) << " " << get<1>(nearest_pos5_mircle_pos.front().pos) << " " << get<2>(nearest_pos5_mircle_pos.front().pos) << " "
                                 << endl;
                            move(allay.id, nearest_pos5_mircle_pos.front().pos);
                        }
                    }
                }
                else if (allay.type == pristest_str)
                {
                    //牧师 朝向覆盖己方单位最多的（非致命）点移动 非致命需要斟酌
                    if (allay.can_move)
                    {
                        //朝向能覆盖最多己方单位的最靠近己方神迹的点行动
                        vector<pos_with_value> most_cover_my_unit_pos;
                        for (auto pos : reach_pos_list)
                        {
                            auto unit_list = units_in_range(pos, 3, map, my_camp);
                            int cover_num = unit_list.size();
                            pos_with_value temp;
                            temp.pos = pos;
                            temp.value1 = cover_num;
                            temp.value2 = cube_distance(pos, miracle_pos);
                            most_cover_my_unit_pos.push_back(temp);
                        }
                        sort(most_cover_my_unit_pos.begin(), most_cover_my_unit_pos.end(), [&](pos_with_value a, pos_with_value b) {
                            if (a.value1 != b.value1)
                            {
                                return a.value1 > b.value1;
                            }
                            else
                            {
                                return a.value2 < b.value2;
                            }
                        });
                        if (!most_cover_my_unit_pos.empty())
                        {
                            cerr << "move id " << allay.id << " to " << get<0>(most_cover_my_unit_pos.front().pos) << " " << get<1>(most_cover_my_unit_pos.front().pos) << " " << get<2>(most_cover_my_unit_pos.front().pos) << " "
                                 << endl;
                            move(allay.id, most_cover_my_unit_pos.front().pos);
                        }
                    }
                }
            }
        }
    }
    else if (attack_mode == PROTECT)
    {
        for (auto allay : my_allay_list)
        {
            //准备每个距离可以到达的点和所有可达的点
            auto reach_pos_with_dis = reachable(allay, map);
            vector<Pos> reach_pos_list;
            for (const auto &reach_pos : reach_pos_with_dis)
            {
                for (auto pos : reach_pos)
                    reach_pos_list.push_back(pos);
            }
            if (allay.can_move && allay.type != pristest_str)
            {
                //第一优先级 占领驻扎点
                for (auto pos : reach_pos_list)
                {
                    int barrack_status = checkBarrack(pos);
                    if (barrack_status == -1 || barrack_status == enemy_camp)
                    {
                        if (getUnitByPos(pos, false).id == -1)
                        { //该驻扎点可以占领
                            cerr << "move id " << allay.id << " to " << get<0>(pos) << " " << get<1>(pos) << " " << get<2>(pos) << " "
                                 << endl;
                            move(allay.id, pos);
                        }
                    }
                }
            }
            if (allay.can_move)
            {
                //攻击状态第二优先级
                if (allay.type == sworderman_str || allay.type == inferno_str)
                {
                    //剑士 或者地狱火 找到该单位id 朝向能打到这个单位的位置移动
                    //否则50 % 朝向距离该位置最近的位置移动
                    //否则朝向能打到对方最多的位置移动
                    Unit target;
                    int target_id = -1;
                    for (auto enemy : enemy_allay_list)
                    {
                        if (cube_distance(enemy.pos, miracle_pos) <= 2)
                        {
                            target_id = enemy.id;
                            target = enemy;
                            break;
                        }
                    }
                    if (target_id != -1)
                    {
                        for (auto pos : reach_pos_list)
                        {
                            int dis = cube_distance(pos, target.pos);
                            if (dis >= allay.atk_range[0] && dis <= allay.atk_range[1])
                            {
                                //找到能打到侵犯基地的单位的位置
                                cerr << "move id " << allay.id << " to " << get<0>(pos) << " " << get<1>(pos) << " " << get<2>(pos) << " "
                                     << endl;
                                move(allay.id, pos);
                                break;
                            }
                        }
                    }
                    if (allay.can_move && target_id != -1)
                    {
                        //朝向距离这个单位最近的位置移动
                        vector<pos_with_value> nearest_enemy_attack_pos;
                        for (auto pos : reach_pos_list)
                        {

                            int dis = cube_distance(pos, target.pos);
                            pos_with_value temp;
                            temp.pos = pos;
                            temp.value1 = dis;
                            temp.value2 = 0;
                            nearest_enemy_attack_pos.push_back(temp);
                        }
                        sort(nearest_enemy_attack_pos.begin(), nearest_enemy_attack_pos.end(), [&](pos_with_value a, pos_with_value b) {
                            return a.value1 < b.value1;
                        });
                        if (!nearest_enemy_attack_pos.empty())
                        {
                            cerr << "move id " << allay.id << " to " << get<0>(nearest_enemy_attack_pos.front().pos) << " " << get<1>(nearest_enemy_attack_pos.front().pos) << " " << get<2>(nearest_enemy_attack_pos.front().pos) << " "
                                 << endl;
                            move(allay.id, nearest_enemy_attack_pos.front().pos);
                        }
                    }
                    if (allay.can_move)
                    {
                        //找不到target，朝向能打到最多人的位置前进
                        vector<pos_with_value> max_enemy_attack_pos;
                        for (auto pos : reach_pos_list)
                        {
                            int enemy_conut = 0; //如果走到该pos能打到敌方多少单位
                            for (auto enemy : enemy_allay_list)
                            {
                                if (enemy.flying == true && allay.atk_flying == true)
                                {
                                    int dis = cube_distance(pos, enemy.pos);
                                    if (dis >= allay.atk_range[0] && dis <= allay.atk_range[1])
                                    {
                                        enemy_conut++;
                                    }
                                }
                                else
                                {
                                    int dis = cube_distance(pos, enemy.pos);
                                    if (dis >= allay.atk_range[0] && dis <= allay.atk_range[1])
                                    {
                                        enemy_conut++;
                                    }
                                }
                            }
                            pos_with_value temp;
                            temp.pos = pos;
                            temp.value1 = enemy_conut;
                            temp.value2 = 0;
                            max_enemy_attack_pos.push_back(temp);
                        }
                        sort(max_enemy_attack_pos.begin(), max_enemy_attack_pos.end(), [&](pos_with_value a, pos_with_value b) {
                            return a.value1 > b.value1;
                        });
                        if (!max_enemy_attack_pos.empty() && max_enemy_attack_pos.front().value1 > 0)
                        {
                            cerr << "move id " << allay.id << " to " << get<0>(max_enemy_attack_pos.front().pos) << " " << get<1>(max_enemy_attack_pos.front().pos) << " " << get<2>(max_enemy_attack_pos.front().pos) << " "
                                 << endl;
                            move(allay.id, max_enemy_attack_pos.front().pos);
                        }
                    }
                }
                else if (allay.type == archer_str)
                {
                    //弓箭手 找到该单位id 朝向能打到这个单位的位置移动
                    Unit target;
                    int target_id = -1;
                    for (auto enemy : enemy_allay_list)
                    {
                        if (cube_distance(enemy.pos, miracle_pos) <= 2)
                        {
                            target_id = enemy.id;
                            target = enemy;
                            break;
                        }
                    }
                    if (target_id != -1)
                    {
                        for (auto pos : reach_pos_list)
                        {
                            int dis = cube_distance(pos, target.pos);
                            if (dis >= allay.atk_range[0] && dis <= allay.atk_range[1])
                            {
                                //找到能打到侵犯基地的单位的位置
                                cerr << "move id " << allay.id << " to " << get<0>(pos) << " " << get<1>(pos) << " " << get<2>(pos) << " "
                                     << endl;
                                move(allay.id, pos);
                                break;
                            }
                        }
                    }
                }
                else if (allay.type == pristest_str)
                {
                    //牧师 朝向覆盖己方单位最多的（非致命）点移动 非致命需要斟酌
                    if (allay.can_move)
                    {
                        //朝向能覆盖最多己方单位的最靠近己方神迹的点行动
                        vector<pos_with_value> most_cover_my_unit_pos;
                        for (auto pos : reach_pos_list)
                        {
                            auto unit_list = units_in_range(pos, 3, map, my_camp);
                            int cover_num = unit_list.size();
                            pos_with_value temp;
                            temp.pos = pos;
                            temp.value1 = cover_num;
                            temp.value2 = cube_distance(pos, miracle_pos);
                            most_cover_my_unit_pos.push_back(temp);
                        }
                        sort(most_cover_my_unit_pos.begin(), most_cover_my_unit_pos.end(), [&](pos_with_value a, pos_with_value b) {
                            if (a.value1 != b.value1)
                            {
                                return a.value1 > b.value1;
                            }
                            else
                            {
                                return a.value2 < b.value2;
                            }
                        });
                        if (!most_cover_my_unit_pos.empty())
                        {
                            cerr << "move id " << allay.id << " to " << get<0>(most_cover_my_unit_pos.front().pos) << " " << get<1>(most_cover_my_unit_pos.front().pos) << " " << get<2>(most_cover_my_unit_pos.front().pos) << " "
                                 << endl;
                            move(allay.id, most_cover_my_unit_pos.front().pos);
                        }
                    }
                }
            }
        }
    }
}
void AI::artifacts_task()
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
void AI::summon_task()
{
    auto my_allay_list = getUnitsByCamp(my_camp);
    sort(my_allay_list.begin(), my_allay_list.end(), cmp);
    //按照剑士，地狱火，弓箭手，牧师排序，相同类型距离基地近的排在前面
    auto enemy_allay_list = getUnitsByCamp(enemy_camp);
    auto summon_pos_list = getSummonPosByCamp(my_camp);
    vector<Pos> available_summon_pos_list;
    for (auto pos : summon_pos_list)
    {
        auto unit_on_pos_ground = getUnitByPos(pos, false);
        if (unit_on_pos_ground.id == -1)
            available_summon_pos_list.push_back(pos);
    }

    if (attack_mode == ATTACK)
    {
        int mana = players[my_camp].mana;
        auto deck = players[my_camp].creature_capacity;
        ::map<string, int> available_count;
        for (const auto &card_unit : deck)
            available_count[card_unit.type] = card_unit.available_count;
        //在法力值够用的情况下持续生成弓箭手
        while (available_count[archer_str] > 0 && mana > (CARD_DICT.at(archer_str)[gj_summon_level].cost))
        {
            vector<pos_with_value> undead_nearest_enemy_pos;
            for (auto pos : summon_pos_list)
            {
                if (getUnitByPos(pos, false).id == -1)
                {
                    int max_damage = 0;
                    for (auto enemy : enemy_allay_list)
                    {
                        int dis = cube_distance(pos, enemy.pos);
                        if (dis >= enemy.atk_range[0] && dis <= enemy.atk_range[1])
                        {
                            if (enemy.atk > max_damage)
                            {
                                max_damage = enemy.atk;
                            }
                        }
                    }
                    //非致命
                    if (max_damage < CARD_DICT.at(archer_str)[gj_summon_level].max_hp)
                    {
                        pos_with_value temp;
                        temp.pos = pos;
                        temp.value1 = cube_distance(pos, enemy_pos);
                        temp.value2 = 0;
                        undead_nearest_enemy_pos.push_back(temp);
                    }
                }
            }
            sort(undead_nearest_enemy_pos.begin(), undead_nearest_enemy_pos.end(), [&](pos_with_value a, pos_with_value b) {
                a.value1 < b.value1;
            });
            if (!undead_nearest_enemy_pos.empty())
            {
                mana -= CARD_DICT.at(archer_str)[gj_summon_level].cost;
                available_count[archer_str] -= 1;
                cerr << "summon " << archer_str << " level " << gj_summon_level << " at " << get<0>(undead_nearest_enemy_pos.front().pos) << " " << get<1>(undead_nearest_enemy_pos.front().pos) << " " << get<2>(undead_nearest_enemy_pos.front().pos) << " " << endl;
                summon(archer_str, gj_summon_level, undead_nearest_enemy_pos.front().pos);
            }
            //召唤弓箭手 完
        }
        //在法力值够用的情况下持续生成牧师
        while (available_count[pristest_str] > 0 && mana > (CARD_DICT.at(pristest_str)[ms_summon_level].cost))
        {
            vector<pos_with_value> most_cover_my_unit_pos;
            for (auto pos : summon_pos_list)
            {
                if (getUnitByPos(pos, false).id == -1)
                {
                    auto unit_list = units_in_range(pos, 3, map, my_camp);
                    int cover_num = unit_list.size();
                    pos_with_value temp;
                    temp.pos = pos;
                    temp.value1 = cover_num;
                    temp.value2 = cube_distance(pos, miracle_pos);
                    most_cover_my_unit_pos.push_back(temp);
                }
            }
            sort(most_cover_my_unit_pos.begin(), most_cover_my_unit_pos.end(), [&](pos_with_value a, pos_with_value b) {
                if (a.value1 != b.value1)
                {
                    return a.value1 > b.value1;
                }
                else
                {
                    return a.value2 < b.value2;
                }
            });
            if (!most_cover_my_unit_pos.empty())
            {
                mana -= CARD_DICT.at(pristest_str)[ms_summon_level].cost;
                available_count[pristest_str] -= 1;
                cerr << "summon " << archer_str << " level " << gj_summon_level << " at " << get<0>(most_cover_my_unit_pos.front().pos) << " " << get<1>(most_cover_my_unit_pos.front().pos) << " " << get<2>(most_cover_my_unit_pos.front().pos) << " " << endl;

                summon(pristest_str, ms_summon_level, most_cover_my_unit_pos.front().pos);
            }
            //召唤牧师 完
        }
        //在法力够用的情况下持续召唤剑士
        while (available_count[sworderman_str] > 0 && mana > (CARD_DICT.at(sworderman_str)[js_summon_level].cost))
        {
            vector<pos_with_value> undead_nearest_enemy_pos;
            for (auto pos : summon_pos_list)
            {
                if (getUnitByPos(pos, false).id == -1)
                {
                    int max_damage = 0;
                    for (auto enemy : enemy_allay_list)
                    {
                        int dis = cube_distance(pos, enemy.pos);
                        if (dis >= enemy.atk_range[0] && dis <= enemy.atk_range[1])
                        {
                            if (enemy.atk > max_damage)
                            {
                                max_damage = enemy.atk;
                            }
                        }
                    }
                    //非致命
                    if (max_damage < CARD_DICT.at(sworderman_str)[js_summon_level].max_hp)
                    {
                        pos_with_value temp;
                        temp.pos = pos;
                        temp.value1 = cube_distance(pos, enemy_pos);
                        temp.value2 = 0;
                        undead_nearest_enemy_pos.push_back(temp);
                    }
                }
            }
            sort(undead_nearest_enemy_pos.begin(), undead_nearest_enemy_pos.end(), [&](pos_with_value a, pos_with_value b) {
                a.value1 < b.value1;
            });
            if (!undead_nearest_enemy_pos.empty())
            {
                mana -= CARD_DICT.at(sworderman_str)[js_summon_level].cost;
                available_count[sworderman_str] -= 1;
                cerr << "summon " << archer_str << " level " << gj_summon_level << " at "
                     << get<0>(undead_nearest_enemy_pos.front().pos) << " "
                     << get<1>(undead_nearest_enemy_pos.front().pos) << " "
                     << get<2>(undead_nearest_enemy_pos.front().pos) << " "
                     << endl;
                summon(sworderman_str, js_summon_level, undead_nearest_enemy_pos.front().pos);
            }
            //召唤剑士 完
        }
    }
    //==================DENFENSE===SUMMON===============
    else if (attack_mode == DEFENSE)
    {
        int mana = players[my_camp].mana;
        auto deck = players[my_camp].creature_capacity;
        ::map<string, int> available_count;
        for (const auto &card_unit : deck)
            available_count[card_unit.type] = card_unit.available_count;
        //在法力值够用的情况下持续生成弓箭手，在最靠近基地的地方
        while (available_count[archer_str] > 0 && mana > (CARD_DICT.at(archer_str)[gj_summon_level].cost))
        {
            vector<pos_with_value> undead_nearest_miracle_pos;
            for (auto pos : summon_pos_list)
            {
                if (getUnitByPos(pos, false).id == -1)
                {
                    int max_damage = 0;
                    for (auto enemy : enemy_allay_list)
                    {
                        int dis = cube_distance(pos, enemy.pos);
                        if (dis >= enemy.atk_range[0] && dis <= enemy.atk_range[1])
                        {
                            if (enemy.atk > max_damage)
                            {
                                max_damage = enemy.atk;
                            }
                        }
                    }
                    //非致命
                    if (max_damage < CARD_DICT.at(archer_str)[gj_summon_level].max_hp)
                    {
                        pos_with_value temp;
                        temp.pos = pos;
                        temp.value1 = cube_distance(pos, miracle_pos); //
                        temp.value2 = 0;
                        undead_nearest_miracle_pos.push_back(temp);
                    }
                }
            }
            sort(undead_nearest_miracle_pos.begin(), undead_nearest_miracle_pos.end(), [&](pos_with_value a, pos_with_value b) {
                a.value1 < b.value1;
            });
            if (!undead_nearest_miracle_pos.empty())
            {
                mana -= CARD_DICT.at(archer_str)[gj_summon_level].cost;
                available_count[archer_str] -= 1;
                cerr << "summon " << archer_str << " level " << gj_summon_level << " at "
                     << get<0>(undead_nearest_miracle_pos.front().pos) << " "
                     << get<1>(undead_nearest_miracle_pos.front().pos) << " "
                     << get<2>(undead_nearest_miracle_pos.front().pos) << " "
                     << endl;
                summon(archer_str, gj_summon_level, undead_nearest_miracle_pos.front().pos);
            }
            //召唤弓箭手 完
        }
        //在法力值够用的情况下持续生成牧师，在最大覆盖的靠近基地的地方
        while (available_count[pristest_str] > 0 && mana > (CARD_DICT.at(pristest_str)[ms_summon_level].cost))
        {
            vector<pos_with_value> most_cover_my_unit_pos;
            for (auto pos : summon_pos_list)
            {
                if (getUnitByPos(pos, false).id == -1)
                {
                    auto unit_list = units_in_range(pos, 3, map, my_camp);
                    int cover_num = unit_list.size();
                    pos_with_value temp;
                    temp.pos = pos;
                    temp.value1 = cover_num;
                    temp.value2 = cube_distance(pos, miracle_pos); //就改了这个
                    most_cover_my_unit_pos.push_back(temp);
                }
            }
            sort(most_cover_my_unit_pos.begin(), most_cover_my_unit_pos.end(), [&](pos_with_value a, pos_with_value b) {
                if (a.value1 != b.value1)
                {
                    return a.value1 > b.value1;
                }
                else
                {
                    return a.value2 < b.value2;
                }
            });
            if (!most_cover_my_unit_pos.empty())
            {
                mana -= CARD_DICT.at(pristest_str)[ms_summon_level].cost;
                available_count[pristest_str] -= 1;
                cerr << "summon " << archer_str << " level " << gj_summon_level << " at "
                     << get<0>(most_cover_my_unit_pos.front().pos) << " "
                     << get<1>(most_cover_my_unit_pos.front().pos) << " "
                     << get<2>(most_cover_my_unit_pos.front().pos) << " "
                     << endl;
                summon(pristest_str, ms_summon_level, most_cover_my_unit_pos.front().pos);
            }
            //召唤牧师 完
        }
        //在法力够用的情况下持续召唤剑士
        while (available_count[sworderman_str] > 0 && mana > (CARD_DICT.at(sworderman_str)[js_summon_level].cost))
        {
            vector<pos_with_value> undead_nearest_miracle_pos;
            for (auto pos : summon_pos_list)
            {
                if (getUnitByPos(pos, false).id == -1)
                {
                    int max_damage = 0;
                    for (auto enemy : enemy_allay_list)
                    {
                        int dis = cube_distance(pos, enemy.pos);
                        if (dis >= enemy.atk_range[0] && dis <= enemy.atk_range[1])
                        {
                            if (enemy.atk > max_damage)
                            {
                                max_damage = enemy.atk;
                            }
                        }
                    }
                    //非致命
                    if (max_damage < CARD_DICT.at(sworderman_str)[js_summon_level].max_hp)
                    {
                        pos_with_value temp;
                        temp.pos = pos;
                        temp.value1 = cube_distance(pos, miracle_pos); //就改了这个
                        temp.value2 = 0;
                        undead_nearest_miracle_pos.push_back(temp);
                    }
                }
            }
            sort(undead_nearest_miracle_pos.begin(), undead_nearest_miracle_pos.end(), [&](pos_with_value a, pos_with_value b) {
                a.value1 < b.value1;
            });
            if (!undead_nearest_miracle_pos.empty())
            {
                mana -= CARD_DICT.at(sworderman_str)[js_summon_level].cost;
                available_count[sworderman_str] -= 1;
                cerr << "summon " << archer_str << " level " << gj_summon_level << " at "
                     << get<0>(undead_nearest_miracle_pos.front().pos) << " "
                     << get<1>(undead_nearest_miracle_pos.front().pos) << " "
                     << get<2>(undead_nearest_miracle_pos.front().pos) << " "
                     << endl;
                summon(sworderman_str, js_summon_level, undead_nearest_miracle_pos.front().pos);
            }
            //召唤剑士 完
        }
    }
    else if (attack_mode == PROTECT)
    {
        int mana = players[my_camp].mana;
        auto deck = players[my_camp].creature_capacity;
        ::map<string, int> available_count;
        for (const auto &card_unit : deck)
            available_count[card_unit.type] = card_unit.available_count;
        Unit target;
        int target_id = -1;
        //找到危险目标
        for (auto enemy : enemy_allay_list)
        {
            if (cube_distance(enemy.pos, miracle_pos) <= 2)
            {
                target_id = enemy.id;
                target = enemy;
            }
        }
        if (target_id != -1 && target.flying == false)
        {
            for (auto pos : summon_pos_list)
            {
                if (getUnitByPos(pos, false).id == -1 && cube_distance(target.pos, pos) == 1)
                {
                    if (available_count[sworderman_str] > 0 && mana > (CARD_DICT.at(sworderman_str)[js_summon_level].cost))
                    {
                        mana -= CARD_DICT.at(sworderman_str)[js_summon_level].cost;
                        available_count[sworderman_str] -= 1;
                        cerr << "summon " << archer_str << " level " << gj_summon_level << " at "
                             << get<0>(pos) << " "
                             << get<1>(pos) << " "
                             << get<2>(pos) << " "
                             << endl;
                        summon(sworderman_str, js_summon_level, pos);
                    }
                }
            }
        }
        else if (target_id != -1 && target.flying == true)
        {
            //如果是飞行生物则找弓箭手能打到的位置
            for (auto pos : summon_pos_list)
            {
                int dis = cube_distance(pos, target.pos);
                if (getUnitByPos(pos, false).id == -1 && dis >= 3 && dis <= 4)
                {
                    if (available_count[archer_str] > 0 && mana > (CARD_DICT.at(archer_str)[gj_summon_level].cost))
                    {
                        mana -= CARD_DICT.at(archer_str)[gj_summon_level].cost;
                        available_count[archer_str] -= 1;
                        cerr << "summon " << archer_str << " level " << gj_summon_level << " at "
                             << get<0>(pos) << " "
                             << get<1>(pos) << " "
                             << get<2>(pos) << " "
                             << endl;
                        summon(archer_str, gj_summon_level, pos);
                    }
                }
            }
        }
        //剩下的体力生成剑士
        while (available_count[sworderman_str] > 0 && mana > (CARD_DICT.at(sworderman_str)[js_summon_level].cost))
        {
            vector<pos_with_value> undead_nearest_miracle_pos;
            for (auto pos : summon_pos_list)
            {
                if (getUnitByPos(pos, false).id == -1)
                {
                    int max_damage = 0;
                    for (auto enemy : enemy_allay_list)
                    {
                        int dis = cube_distance(pos, enemy.pos);
                        if (dis >= enemy.atk_range[0] && dis <= enemy.atk_range[1])
                        {
                            if (enemy.atk > max_damage)
                            {
                                max_damage = enemy.atk;
                            }
                        }
                    }
                    //非致命
                    if (max_damage < CARD_DICT.at(sworderman_str)[js_summon_level].max_hp)
                    {
                        pos_with_value temp;
                        temp.pos = pos;
                        temp.value1 = cube_distance(pos, miracle_pos); //就改了这个
                        temp.value2 = 0;
                        undead_nearest_miracle_pos.push_back(temp);
                    }
                }
            }
            sort(undead_nearest_miracle_pos.begin(), undead_nearest_miracle_pos.end(), [&](pos_with_value a, pos_with_value b) {
                a.value1 < b.value1;
            });
            if (!undead_nearest_miracle_pos.empty())
            {
                mana -= CARD_DICT.at(sworderman_str)[js_summon_level].cost;
                available_count[sworderman_str] -= 1;
                cerr << "summon " << archer_str << " level " << gj_summon_level << " at "
                     << get<0>(undead_nearest_miracle_pos.front().pos) << " "
                     << get<1>(undead_nearest_miracle_pos.front().pos) << " "
                     << get<2>(undead_nearest_miracle_pos.front().pos) << " "
                     << endl;
                summon(sworderman_str, js_summon_level, undead_nearest_miracle_pos.front().pos);
            }
            //召唤剑士 完
        }
    }
}
void AI::play()
{
    cerr << "round: " << round << endl;
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
