#include "attacker.hpp"

static PD pd_gyro(0.75f, -0.04f); // ジャイロ用のPD調節値
static PD pd_cam(0.75f, -0.04f);  // カメラ用のPD調節値

const float line_escape_power = 55.0f; // ライン反応時のモーターの強さ
const float line_trace_power = 75.0f;  // ライントレース時モーターの強さ
const float ir_max_power = 95.0f;      // モーターの強さ

const float goal_escape_dis = 69.0f; // ゴールから逃れる距離

// 回り込み関数
Vector mawarikomi(float max_power, float ball_deg, float ball_dis)
{
    if (!irDetected())
        return Vector(0, 0.0f);

    float ir_x = cosf(radians(ball_deg)) * ball_dis;
    float ir_y = sinf(radians(ball_deg)) * ball_dis;

    Vector vec;

    if (catchSensor.read() == HIGH && fabsf(ball_deg) < 20)
    {
        vec = Vector(0.0f, max_power);
    }
    else if (ir_x > 0.0f && fabsf(ir_y) < 140.0f)
    {
        // rad計算
        float dis_x = smoothShifter(60.0f, 140.0f, 65.0f, 280.0f, ir_y);
        // length計算
        float length_temp;
        if (fabsf(ir_y) < 60.0f)
        {
            length_temp = max_power;
        }
        else
        {
            // length_temp = smoothShifter(60.0f, 150.0f, 50.0f, 95.0f, ir_y);
            length_temp = max_power;
        }

        Vector vec_temp = Vector(dis_x, 0, ir_x, ir_y);
        vec_temp = vec_temp * length_temp / vec_temp.length();

        vec = vec_temp;
    }
    else
    {
        const float r = 250.0f;
        const float pi = (float)PI;

        float ir_rad = (float)radians(ball_deg);
        float ir_dis = ball_dis;

        float rad_temp;
        float length_temp = 85.0f;

        if (r <= ir_dis)
        {
            float alpha = asinf(r / ir_dis);

            if (ir_rad >= 0)
            {
                rad_temp = ir_rad + alpha;
            }
            else // (ir_rad < 0)
            {
                rad_temp = ir_rad - alpha;
            }
        }
        else // (r > ir_dis)
        {
            if (ir_rad >= 0)
            {
                rad_temp = ir_rad + pi / 2;
            }
            else // (ir_rad < 0)
            {
                rad_temp = ir_rad - pi / 2;
            }
        }
        vec = Vector(degrees(rad_temp), length_temp);
    }

    static float smooth_length = 60.0f;
    if (vec.length() >= smooth_length) // ゆっくり加速
    {
        smooth_length += 1.5f;
    }
    else
    {
        smooth_length -= 1.5f;
    }
    smooth_length = constrain(smooth_length, 0.0f, 95.0f);

    // Vector final_vec = Vector(vec.deg(), smooth_length);
    Vector final_vec = vec;

    return final_vec;
}

//// アタッカーメイン ////
void playAttacker(Attacker::Mode mode)
{
    //// ゴールの位置更新
    goalPositionCheckAtMode(mode);

    //// PD制御目標方向確定
    static Timer last_catch_timer;
    bool a = catchSensor.read() == HIGH;
    static bool b = false;
    if (!a && b)
    {
        last_catch_timer.reset();
    }
    b = a;

    if (mode == Attacker::Mode::GYRO)
    {
        motorsPdProcess(&pd_gyro, bnoDeg(), 0);
    }
    else // (mode == Attacker::Mode::YELLOWGOAL) || (mode == Attacker::Mode::BLUEGOAL)
    {
        if (attackGoalDetected())
        {
            int diff = fabsf(normalizeDeg(diffDeg(0, attackGoalDeg())));

            if (catchSensor.read() == HIGH || last_catch_timer.msTime() < 500) // キャッチする時間
            {
                if (!irDetected())
                {
                    motorsPdProcess(&pd_gyro, bnoDeg(), 0);
                }
                else
                {
                    if (diff > 40)
                    {
                        if (attackGoalDeg() < 0)
                        {
                            motorsPdProcess(&pd_cam, bnoDeg(), -40);
                        }
                        else
                        {
                            motorsPdProcess(&pd_cam, bnoDeg(), 40);
                        }
                    }
                    else
                    {
                        motorsPdProcess(&pd_cam, attackGoalDeg(), 0);
                    }
                }
            }
            else
            {
                motorsPdProcess(&pd_gyro, bnoDeg(), 0);
            }
        }
        else
        {
            motorsPdProcess(&pd_gyro, bnoDeg(), 0);
        }
    }

    //// ライン・ライントレース動作決定
    LinePosition current_line_position = linePositionCheck();
    if (current_line_position == LinePosition::None)
    {
        if (lineSideRightDetected())
        {
            if (lineSideLeftDetected())
            {
                current_line_position = LinePosition::Yoko;
            }
            else
            {
                current_line_position = LinePosition::Tate;
            }
        }
        else if (lineSideLeftDetected())
        {
            current_line_position = LinePosition::Tate;
        }
    }

    static bool line_trace_flag = false;
    static Timer line_trace_timer;

    enum LineForm
    {
        LineEscape,    // 普通の回避動作
        LineTraceYoko, // 横線上のトレース
        LineTraceTate, // 縦線上のトレース
        None
    };
    LineForm line_form = LineForm::None;

    static Timer line_timer; // 前回の「検出」からの経過時間を測る

    bool line_detected = (current_line_position != LinePosition::None);
    static bool old_line_detected = false;

    static int line_switching_count = 0;
    static LinePosition memory_last_line_position;

    const unsigned long line_switching_min_time = 10UL;
    const unsigned long line_switching_max_time = 2000UL;

    if (!line_trace_flag)
    {
        if (!line_detected) // ラインを踏んでいない
        {
            if (line_timer.msTime() < 80UL) // ラインを離れてすぐは回避動作を維持
            {
                line_form = LineForm::LineEscape;
            }
            else
            {
                line_form = LineForm::None;
                // ラインに長い時間触れなければカウントをリセット
                if (line_timer.msTime() > line_switching_max_time)
                {
                    line_switching_count = 0;
                }
            }
        }
        else
        {
            line_form = LineForm::LineEscape;

            if (!old_line_detected)
            {
                unsigned long interval = line_timer.msTime();

                // 前回の踏んだ瞬間からの経過時間で判定
                if (interval > line_switching_min_time && interval < line_switching_max_time)
                {
                    line_switching_count++;
                }
                else
                {
                    // 間隔が短すぎるか、長すぎる場合は1に戻す
                    line_switching_count = 1;
                }

                line_timer.reset();
                memory_last_line_position = current_line_position;
            }
        }
    }

    old_line_detected = line_detected; // 記録

    // 2回以上反応でライントレースモードに移行する
    static bool old_line_trace_flag = false;
    if (line_switching_count > 1 && (memory_last_line_position == LinePosition::Tate || memory_last_line_position == LinePosition::Yoko))
    {
        line_trace_flag = true;

        if (!old_line_trace_flag)
        {
            line_trace_timer.reset();
        }
    }
    old_line_trace_flag = line_trace_flag;

    // ライントレースモード解除条件
    if (line_trace_flag)
    {
        if (!irDetected())
        {
            line_trace_flag = false;
            line_switching_count = 0;
        }

        memory_last_line_position = current_line_position;

        if (memory_last_line_position == LinePosition::Yoko)
        {
            if (isMyAttackArea()) // 攻撃エリアでの誤反応を無くす
            {
                line_trace_flag = false;
                line_switching_count = 0;
            }

            if (!isMyDefenceArea()) // 守備エリアでの誤反応を無くす
            {
                line_trace_flag = false;
                line_switching_count = 0;
            }

            // if (current_line_position == LinePosition::Tate || current_line_position == LinePosition::Haji) // ラインポジションが変わったら解除
            // {
            //      line_trace_flag = false;
            //      line_switching_count = 0;
            // }

            float line_ring_deg = (fabsf(fieldDeg()) > 90) ? 0 : 180;

            if (fabsf(diffDeg(line_ring_deg, irDeg())) > 25) // ライン方向とボール方向の差を見て解除
            {
                line_trace_flag = false;
                line_switching_count = 0;
            }
        }
        else // (memory_last_line_position == LinePosition::Tate)
        {
            int line_ring_deg = (fieldDeg() > 0) ? -90 : 90;

            if (fabsf(diffDeg(line_ring_deg, irDeg())) > 110) // ライン方向とボール方向の差を見て解除
            {
                line_trace_flag = false;
                line_switching_count = 0;
            }

            if (isMyAttackArea()) // 攻撃エリアでの誤反応を無くす
            {
                line_trace_flag = false;
                line_switching_count = 0;
            }

            // if (current_line_position == LinePosition::Yoko || current_line_position == LinePosition::Haji) // ラインポジションが変わったら解除
            // {
            //      line_trace_flag = false;
            //      line_switching_count = 0;
            // }
        }
    }

    // ラインフォームをライントレースモードとして書き換え
    if (line_trace_flag)
    {
        if (memory_last_line_position == LinePosition::Yoko)
        {
            line_form = LineForm::LineTraceYoko;
        }
        else
        {
            line_form = LineForm::LineTraceTate;
        }
    }

    Serial.print("posi:");
    Serial.print((current_line_position == LinePosition::Yoko) ? "Yoko" : (current_line_position == LinePosition::Tate) ? "Tate"
                                                                                                                        : "None");

    Serial.print(", sw_cnt:");
    Serial.print(line_switching_count);

    Serial.print(", last_posi:");
    Serial.print((memory_last_line_position == LinePosition::Yoko) ? "Yoko" : (memory_last_line_position == LinePosition::Tate) ? "Tate"
                                                                                                                                : "None");

    Serial.print(", line_form:");
    Serial.print((line_form == LineForm::LineEscape) ? "Escape" : (line_form == LineForm::LineTraceTate) ? "Tate"
                                                              : (line_form == LineForm::LineTraceYoko)   ? "Yoko"
                                                                                                         : "None");

    Serial.print(", line_timer:" + String(line_timer.msTime()));

    Serial.println(", linetrace_flag:" + String(line_trace_flag));

    //// 制御
    bool kick_signal = false; // キックしたいture

    if (line_form == LineForm::LineEscape)
    {
        if (irDetected())
        {
            Vector vec = mawarikomi(ir_max_power, irDeg(), irDis());
            if (fabsf(diffDeg(vec.deg(), fieldDeg())) < 90)
            {
                motorsMove(vec.deg(), line_escape_power);
            }
            else
            {
                motorsMove(fieldDeg(), line_escape_power);
            }
        }
        else
        {
            motorsMove(fieldDeg(), line_escape_power);
        }
    }
    else if (line_form == LineForm::LineTraceYoko)
    {
        if (defenceGoalDis() < goal_escape_dis)
        {
            motorsMove(defenceGoalDeg() + 180, line_trace_power);
        }
        else if (lineRingDetected())
        {
            float trace_deg = nearSessenDeg(defenceGoalDeg(), irDeg());
            Vector vec = lineTrace(trace_deg, line_trace_power - 10.0f, 10.0f);
            motorsVectorMove(&vec);
        }
        else
        {
            Vector vec = mawarikomi(ir_max_power, irDeg(), irDis());
            vec = vec * line_trace_power / vec.length();
            motorsVectorMove(&vec);
        }
    }
    else if (line_form == LineForm::LineTraceTate)
    {
        if (lineRingDetected())
        {
            if (lineRingDis() > 60 && fabsf(diffDeg(lineRingDeg(), fieldDeg())) > 90)
            {
                if (catchSensor.read() == HIGH)
                {
                    kick_signal = true;
                }

                motorsMove(nearSessenDeg(lineRingDeg(), irDeg()), line_trace_power);
            }
            else
            {
                motorsMove(fieldDeg(), line_trace_power);
            }
        }
        else
        {
            if (catchSensor.read() == HIGH)
            {
                kick_signal = true;
                motorsMove(irDeg(), line_trace_power);
            }
            else
            {
                Vector vec = mawarikomi(line_trace_power, irDeg(), irDis());
                motorsVectorMove(&vec);
            }
        }
    }
    //// カメラによるゴール判定
    else if (attackGoalDetected() && attackGoalDis() < goal_escape_dis) // ゴール内部に侵入してアウトオブバウンズするようだったら60をどっちもあげてください
    {
        motorsMove(attackGoalDeg() + 180, line_escape_power);
    }
    else if (defenceGoalDetected() && defenceGoalDis() < goal_escape_dis)
    {
        motorsMove(defenceGoalDeg() + 180, line_escape_power);
    }
    //// ボール回り込み
    else if (irDetected())
    {
        // if (goaldetected && goaldis < 80)
        // {
        //     int goal_to_ball_deg = -diffDeg(goaldeg, irDeg());
        //     Vector vec = mawarikomi(ir_max_power, goaldis, goal_to_ball_deg, irDis());
        //     vec = Vector(vec.deg() + goaldeg, vec.length());
        //     motorsVectorMove(&vec);
        // }
        // else
        // {
        //     Vector vec = mawarikomi(ir_max_power, goaldis, irDeg(), irDis());
        //     motorsVectorMove(&vec);
        // }

        // motorsMove(0,0);
        Vector vec = mawarikomi(ir_max_power, irDeg(), irDis());
        motorsVectorMove(&vec);
    }
    else
    {
        motorsPdMove();
    }

    //// キッカー制御
    static Timer catching_timer;
    static bool old_catch = false;
    bool current_catch = (catchSensor.read() == HIGH);
    if (current_catch)
    {
        if (!old_catch)
        {
            catching_timer.reset();
        }

        if (mode == Attacker::Mode::GYRO)
        {
            if (catching_timer.msTime() > 80) // キックする時間
            {
                kicker1.kick();
            }
        }
        else // (mode == Attacker::Mode::YELLOWGOAL) || (mode == Attacker::Mode::BLUEGOAL)
        {
            if (kick_signal) // ライントレース時のキック指示は問答無用でキック
            {
                kicker1.kick();
            }
            else
            {
                const float y = 90.0f; // キッカー判定用の黄色ゴール距離
                const float b = 90.0f; // キッカー判定用の青色ゴール距離
                int kick_at_goal_dis = (mode == Attacker::Mode::YELLOWGOAL) ? y : b;

                if (attackGoalDetected()) //&& attackGoalDis() <= kick_at_goal_dis)
                {
                    kicker1.kick();
                }
                else // if (attackGoalDis() > kick_at_goal_dis)
                {
                    if (catching_timer.msTime() > 10)
                    {
                        kicker1.kick();
                    }
                }
            }
        }
    }
    old_catch = current_catch; // 記録
}