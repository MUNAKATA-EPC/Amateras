#include "defenderVer4.hpp"

static PD pd_gyro(0.75f, -0.04f); // ジャイロ用のPD調節値
static PD pd_cam(0.75f, -0.04f);  // カメラ用のPD調節値

// ディフエンダーのフォーム
enum DfForm
{
    ATTACKER,           // playAttacker関数による攻撃フォーム
    DEFENCE,            // 通常の守備フォーム
    DEFENCE_LINE_HOSEI, // 自分陣地でラインから離れた場合一時的にラインに戻ろうとするフォーム
    KIKAN               // 自分陣地に帰還するフォーム
};

// ディフエンダー用ラントレース関数
Vector dfLineTrace(LinePosition posi, float move_deg, float move_power, float line_trace_power)
{
    if (!lineRingDetected())
        return Vector(0, 0);

    move_deg = normalizeDeg(move_deg); // 正規化

    float df_move_deg, df_move_power, df_line_trace_power;
    if (posi == Haji)
    {
        if (defenceGoalDeg() > 0) // コート右側
        {
            if (move_deg < -90 || move_deg > 135)
            {
                df_move_deg = 0;
                df_move_power = 0;

                df_line_trace_power = 50;
            }
            else
            {
                df_move_deg = 0;
                df_move_power = move_power * 0.6f;

                df_line_trace_power = line_trace_power;
            }
        }
        else // コート左側
        {
            if (move_deg > 90 || move_deg < -135)
            {
                df_move_deg = 0;
                df_move_power = 0;

                df_line_trace_power = 50;
            }
            else
            {
                df_move_deg = 0;
                df_move_power = move_power * 0.6f;

                df_line_trace_power = line_trace_power;
            }
        }
    }
    else if (posi == Tate)
    {
        if (defenceGoalDeg() > 0) // コート右側
        {
            if (move_deg < -90 || move_deg > 135)
            {
                df_move_deg = 180;
                df_move_power = move_power * 0.6f;
            }
            else
            {
                df_move_deg = 0;
                df_move_power = move_power * 0.6f;
            }

            df_line_trace_power = line_trace_power;
        }
        else // コート左側
        {
            if (move_deg > 90 || move_deg < -135)
            {
                df_move_deg = 180;
                df_move_power = move_power * 0.6f;
            }
            else
            {
                df_move_deg = 0;
                df_move_power = move_power * 0.6f;
            }

            df_line_trace_power = line_trace_power;
        }
    }
    else if (posi == Yoko)
    {
        if (lineSideLeftDetected() && lineSideRightDetected())
        {
            if (move_deg > 0)
            {
                df_move_deg = 90;
                df_move_power = move_power + line_trace_power;
            }
            else
            {
                df_move_deg = -90;
                df_move_power = move_power + line_trace_power;
            }

            df_line_trace_power = 0;
        }
        else
        {
            if (move_deg > 0)
            {
                df_move_deg = 90;
                df_move_power = move_power;
            }
            else
            {
                df_move_deg = -90;
                df_move_power = move_power;
            }

            df_line_trace_power = line_trace_power;
        }
    }
    else // if (posi == None)
    {
        df_move_deg = 0;
        df_move_power = 0;

        df_line_trace_power = 0;
    }

    Vector vec;
    vec = lineTrace(df_move_deg, df_move_power, df_line_trace_power);

    return vec;
}

//// ディフエンダーメイン ////
void playDefenderVer4(Defender::Mode mode)
{
    //// 自分ゴールと相手ゴールの確認
    goalPositionCheckDfMode(mode);

    //// フォーム決定
    DfForm df_form;
    LinePosition line_position = linePositionCheck();

    // 守備エリア決定
    bool is_my_defence_area = isMyDefenceArea();
    if (fabs(fieldDeg()) < 90)
    {
        if (line_position == LinePosition::Yoko)
        {
            if (fabsf(defenceGoalDeg()) < 120)
            {
                is_my_defence_area = false;
            }
        }

        if (fabsf(defenceGoalDeg()) < 98)
        {
            is_my_defence_area = false;
        }
    }

    // 攻撃フォーム切り替え条件
    static bool my_defence_antei_condition = false; // 守備フォーム かつ ボールとゴールの線分上にいる時true
    static bool old_my_defence_antei_condition = false;

    static Timer my_defence_antei_condition_timer; // 守備フォーム かつ ボールとゴールの線分上にいる時間計測用
    if (!old_my_defence_antei_condition && my_defence_antei_condition)
    {
        my_defence_antei_condition_timer.reset();
    }

    static Timer attack_form_timer; // ATTAKCERフォーム継続時間
    static bool attack_form_flag = false;
    if (!attack_form_flag)
    {
        if (my_defence_antei_condition && my_defence_antei_condition_timer.msTime() > 2500)
        {
            attack_form_flag = true;
            my_defence_antei_condition = false;
            attack_form_timer.reset();
        }
    }

    old_my_defence_antei_condition = my_defence_antei_condition; // 記録
    my_defence_antei_condition = false;                          // リセット

    // 攻撃フォーム取り消し条件
    if (attack_form_flag)
    {
        // ボールが見えなかったら取り消し
        if (!irDetected())
        {
            attack_form_flag = false;
        }
        // ゴール付近以外でラインが反応したら取り消し
        else if (lineRingDetected() && !isMyDefenceArea() && !isMyAttackArea())
        {
            attack_form_flag = false;
        }
        // 5秒以上たったらATTACKERフォーム取り消し
        else if (attack_form_timer.msTime() > 5000UL)
        {
            attack_form_flag = false;
        }
    }

    // 代入
    if (attack_form_flag)
    {
        df_form = DfForm::ATTACKER; // ATTACKERフォーム
    }
    else if (is_my_defence_area)
    {
        if (lineRingDetected())
        {
            df_form = DfForm::DEFENCE; // DEFENCEフォーム
        }
        else if (false) // (lineRingLastDetectingTime() < 5000UL && defenceGoalDetected())
        {
            df_form = DfForm::DEFENCE_LINE_HOSEI; // DEFENCE_LINE_HOSEIフォーム
        }
        else
        {
            df_form = DfForm::KIKAN; // KIKANフォーム
        }
    }
    else // KIKAN処理
    {
        df_form = DfForm::KIKAN; // KIKANフォーム
    }

    //// 守備制御
    // 移動方向決定
    static DfForm old_df_form;

    Vector move_vec;
    if (df_form == DfForm::ATTACKER)
    {
        if (lineRingDetected() && is_my_defence_area && fabsf(diffDeg(irDeg(), defenceGoalDeg())) > 70)
        {
            move_vec = Vector(irDeg(), 60);
        }
        else
        {
            playAttacker((mode == Defender::Mode::YELLOWGOAL) ? Attacker::Mode::YELLOWGOAL : Attacker::Mode::BLUEGOAL);
            old_df_form = df_form; // 記録
            return;
        }
    }
    else if (df_form == DfForm::DEFENCE)
    {
        static bool old_ir_detected = false;
        if (old_df_form != DfForm::DEFENCE)
        {
            old_ir_detected = false;
        }
        static Timer ir_last_detected_timer;
        if (!irDetected() && old_ir_detected)
        {
            ir_last_detected_timer.reset();
        }

        if (irDetected())
        {
            float defence_goal_deg = defenceGoalDeg();
            float defence_ir_deg = nearSessenDeg(defence_goal_deg, irDeg()); // 守備ゴールの±90度の方向のうち、ボールの方向に近い角度

            float defence_ir_y = irDis() * sinf(radians(diffDeg(defence_goal_deg + 180, irDeg()))); // y成分
            float defence_ir_x = irDis() * cosf(radians(diffDeg(defence_goal_deg + 180, irDeg()))); // x成分

            static MovementAverage y_ave(15);
            float smooth_defence_ir_y = y_ave.add(defence_ir_y);

            if (defence_ir_x > 0 && fabsf(defence_ir_y) < 80)
            {
                if (line_position == LinePosition::Yoko && fabsf(smooth_defence_ir_y) < 80)
                {
                    my_defence_antei_condition = true;
                }

                if (fabsf(defence_ir_y) < 40)
                {
                    move_vec = Vector(lineRingDeg(), 10);
                }
                else
                {
                    float length = smoothShifter(40, 80, 0, 90, fabsf(defence_ir_y));
                    move_vec = dfLineTrace(line_position, defence_ir_deg, length, 10);
                }
            }
            else if (line_position == LinePosition::Yoko && (lineSideRightDetected() || lineSideLeftDetected()) && irDetected())
            {
                if (irDeg() > 0)
                {
                    move_vec = Vector(90, 80);
                }
                else
                {
                    move_vec = Vector(-90, 80);
                }
            }
            else
            {
                move_vec = dfLineTrace(line_position, defence_ir_deg, 90, 10);
            }
        }
        else if (ir_last_detected_timer.everReset() && ir_last_detected_timer.msTime() < 2000)
        {
            move_vec = Vector(lineRingDeg(), 10);
        }
        else
        {
            // 定位置移動
            if (fabsf(defenceGoalDeg()) > 165)
            {
                move_vec = lineTrace(0, 0, 10);
            }
            else
            {
                float teiiti_deg = (defenceGoalDeg() > 0) ? defenceGoalDeg() - 90 : defenceGoalDeg() + 90;
                move_vec = dfLineTrace(line_position, teiiti_deg, 90, 10);
            }
        }

        old_ir_detected = irDetected();
    }
    else if (df_form == DfForm::DEFENCE_LINE_HOSEI)
    {
        if (lineRingLastDetectingTime() < 1000)
        {
            move_vec = Vector(lineRingLastDeg(), 60);
        }
        else
        {
            if (defenceGoalDis() <= 58)
            {
                move_vec = Vector(defenceGoalDeg() + 180, 60);
            }
            else
            {
                move_vec = Vector(defenceGoalDeg(), 60);
            }
            // move_vec = Vector(lineRingLastDeg(), 40);
        }
    }
    else // if (df_form == DfForm::KIKAN)
    {
        if (lineRingDetected())
        {
            // ラインから逃れる動き
            if (fabsf(lineAbsoluteRingDeg()) > 135 &&
                fabsf(fieldDeg()) < 90) // |__(ロボット)__(ゴール)___________| この状況である可能性が高い
            {
                if (fieldDeg() > 0) // コート上で右にいる
                {
                    move_vec = lineTrace(90, 50, 10); // ライントレースする
                }
                else
                {
                    move_vec = lineTrace(-90, 50, 10); // ライントレースする
                }
            }
            else
            {
                move_vec = Vector(fieldDeg(), 70);
            }
        }
        else if (is_my_defence_area)
        {
            if (defenceGoalDis() >= 88)
            {
                // ゴールに向かう動き
                move_vec = Vector(defenceGoalDeg(), 70);
            }
            else
            {
                // ゴールから離れる
                if (defenceGoalDis() < 50)
                {
                    move_vec = Vector(defenceGoalDeg() + 180, 70);
                }
                else
                // ゴールに向かう
                {
                    move_vec = Vector(defenceGoalDeg(), 70);
                }
            }
        }
        else
        {
            if (fieldDeg() > 0)
            {
                move_vec = Vector(165, 70); // 後ろに移動する
            }
            else
            {
                move_vec = Vector(-165, 70); // 後ろに移動する
            }
        }
    }

    old_df_form = df_form; // 記録

    // pd
    motorsPdProcess(&pd_gyro, bnoDeg(), 0);

    motorsVectorMove(&move_vec);
}