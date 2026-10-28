#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <Wire.h>
#include <U8g2lib.h>

#include "infra.hpp"

//// record ////
namespace record
{
    // レコードの並び: [id:1 byte][cycle:1 byte][本文の長さ:1 byte][本文]
    // logは、複数のレコードを続けて格納する一次元配列
    // 各レコードの本文は可変長なので、本文の長さを使って次のレコード位置を求める
    char data[2048];

    // log内で有効なデータが終わる位置（バイト数）
    size_t targetindex = 0;

    // log配列全体の容量（バイト数）
    const size_t size = sizeof(data);

    /**
     * @brief   指定位置にあるレコードのヘッダーを読み取る
     *
     * レコードは [id][cycle][本文の長さ][本文] の順で格納されている
     * 本文が配列の有効範囲内に収まっているかも確認する
     *
     * @param[in]  data       レコードが格納されている配列
     * @param[in]  data_size  配列全体の容量（バイト数）
     * @param[in]  used       配列内で現在有効なデータの長さ（バイト数）
     * @param[in]  idx        読み取るレコードの先頭位置
     * @param[out] id         読み取ったid
     * @param[out] cycle      読み取ったcycle
     * @param[out] text_len   読み取った本文の長さ
     *
     * @return ヘッダーと本文が有効な範囲にあればtrue
     */
    static bool read_header(
        const char *data,
        size_t data_size,
        size_t used,
        size_t idx,
        uint8_t &id,
        uint8_t &cycle,
        uint8_t &text_len)
    {
        // ヘッダー3バイトが有効なデータの範囲にあるか確認する
        if (used > data_size || idx > used || used - idx < 3)
            return false;

        // 1バイトずつヘッダーを読み取る
        id = static_cast<uint8_t>(data[idx]);
        cycle = static_cast<uint8_t>(data[idx + 1]);
        text_len = static_cast<uint8_t>(data[idx + 2]);

        // 本文の長さが、設定された最大サイズを超えていないか確認する
        if (text_len > infra::text_size_max)
            return false;

        // 本文全体が有効なデータの範囲内にあるか確認する
        return text_len <= used - idx - 3;
    }

    /**
     * @brief   log内のすべてのレコードが正しい形式か確認する
     *
     * 先頭からレコードを順番に読み、各レコードの長さを足し合わせる
     * 最後のレコードの終端がlog_targetindexと一致すれば有効とする
     *
     * @return log全体が正しい形式であればtrue
     */
    static bool is_valid()
    {
        // 有効データの長さが配列容量を超えていたら不正
        if (targetindex > size)
            return false;

        size_t pos = 0;

        // posを各レコードの先頭へ進めながら、順番に形式を確認する
        while (pos < targetindex)
        {
            uint8_t id, cycle, text_len;

            if (!read_header(
                    data, size, targetindex, pos,
                    id, cycle, text_len))
            {
                return false;
            }

            // ヘッダー3バイトと本文の長さ分だけ次のレコードへ進む
            pos += 3 + text_len;
        }

        // レコードを読み終えた位置が有効データの末尾と一致するか確認する
        return pos == targetindex;
    }

    /**
     * @brief   指定位置のレコードをlogから削除する
     *
     * 削除したレコードより後ろにあるデータを左へ詰め、
     * log_targetindexを削除したバイト数だけ減らす
     *
     * @param offset 削除するレコードの先頭位置
     *
     * @return レコードを削除できた場合はtrue
     */
    static bool remove_record_at(size_t offset)
    {
        uint8_t id, cycle, text_len;

        // 指定位置からレコードのヘッダーを読み取る
        if (!read_header(
                data, size, targetindex, offset,
                id, cycle, text_len))
        {
            return false;
        }

        const size_t record_size = 3 + text_len;

        // 削除対象より後ろの全データを、削除位置へ移動する
        memmove(
            data + offset,
            data + offset + record_size,
            targetindex - offset - record_size);

        targetindex -= record_size;
        return true;
    }

    /**
     * @brief logの先頭にある最も古いレコードを削除する
     *
     * レコードは追加順に格納されるため、先頭のレコードが最も古い
     *
     * @return レコードを削除できた場合はtrue
     */
    static bool remove_oldest_record()
    {
        return remove_record_at(0);
    }

    /**
     * @brief   新しいレコードをlogの末尾に追加する
     *
     * 空き容量が足りない場合は、先頭にある最も古いレコードから削除して
     * 必要な容量を確保する本文は'\0'を含まないバイト列として格納する
     *
     * @param id        記録の所有者を表すid（friendsまたはmy_id）
     * @param cycle     このidの記録を区別する周期カウント（0～255）
     * @param text      格納する本文
     * @param text_size 格納する本文の長さ（バイト数'\0'は含めない）
     */
    void append(uint8_t id, uint8_t cycle, char *text, size_t text_size)
    {
        // 本文が最大サイズを超えている場合や、本文があるのにポインターが
        // nullptrの場合は記録しない
        if (text_size > infra::text_size_max || (text_size > 0 && text == nullptr))
            return;

        // ヘッダー3バイトと本文を合わせた、今回のレコードのサイズ
        const size_t record_size = 3 + text_size;

        // 1件のレコードが配列容量を超える場合は記録できない
        if (record_size > size)
            return;

        // 追加時に古いレコードを削除しても本文を保てるよう、先に退避する
        // textがlog配列内を指している場合にも対応する
        char text_copy[infra::text_size_max];

        if (text_size > 0)
            memcpy(text_copy, text, text_size);

        // logの内部状態が不正なら、既存データを空として扱う
        if (!is_valid())
            targetindex = 0;

        // 新しいレコードが収まるまで、最も古いレコードを削除する
        while (record_size > size - targetindex)
        {
            if (!remove_oldest_record())
            {
                // 先頭レコードを正しく読めない場合は、破損した状態を初期化する
                targetindex = 0;
                break;
            }
        }

        // ヘッダー（id、cycle、本文の長さ）を書き込む
        data[targetindex++] = static_cast<char>(id);
        data[targetindex++] = static_cast<char>(cycle);
        data[targetindex++] = static_cast<char>(text_size);

        // 本文を書き込み、有効データの末尾を更新する
        if (text_size > 0)
        {
            memcpy(data + targetindex, text_copy, text_size);
            targetindex += text_size;
        }
    }

    /**
     * @brief   指定したidのcycle範囲にあるレコードを削除する
     *
     * 指定したidの記録を新しい順に調べ、最新のend_cycleを見つける
     * そこから古い方向へ戻り、最も近いstart_cycleまでの同じidの記録を削除する
     * 範囲内にある別idの記録は削除せず、順序を保って残す
     *
     * @param id          削除対象のid（friendsまたはmy_id）
     * @param start_cycle 削除範囲の古い側のcycle
     * @param end_cycle   削除範囲の新しい側のcycle
     */
    void deletes(uint8_t id, uint8_t start_cycle, uint8_t end_cycle)
    {
        // 記録形式が壊れている場合は、誤った位置を削除しない
        if (!is_valid())
            return;

        // 記録を先頭から読み、指定idの最新のend_cycleを探す
        // 同じcycleが複数回あれば、後ろにある（より新しい）記録を採用する
        size_t end_offset = 0;
        bool found_end = false;

        for (size_t pos = 0; pos < targetindex;)
        {
            uint8_t record_id, cycle, text_len;
            read_header(
                data, size, targetindex, pos,
                record_id, cycle, text_len);

            if (record_id == id && cycle == end_cycle)
            {
                end_offset = pos;
                found_end = true;
            }

            pos += 3 + text_len;
        }

        // end_cycleの記録がなければ削除対象を決められない
        if (!found_end)
            return;

        // end_cycle以前にある、最も近いstart_cycleを探す
        size_t start_offset = 0;
        bool found_start = false;

        for (size_t pos = 0; pos <= end_offset;)
        {
            uint8_t record_id, cycle, text_len;
            read_header(
                data, size, targetindex, pos,
                record_id, cycle, text_len);

            if (record_id == id && cycle == start_cycle)
            {
                // 後から見つかった一致位置で更新するため、
                // end_cycleに最も近いstart_cycleが残る
                start_offset = pos;
                found_start = true;
            }

            // end_cycleのレコードまで確認したら探索を終える
            if (pos == end_offset)
                break;

            pos += 3 + text_len;
        }

        // end_cycleより古い側にstart_cycleがなければ何も削除しない
        if (!found_start)
            return;

        // 削除対象以外のレコードを、元の順番のまま配列の先頭へ詰める
        size_t write_pos = 0;

        for (size_t pos = 0; pos < targetindex;)
        {
            uint8_t record_id, cycle, text_len;
            read_header(
                data, size, targetindex, pos,
                record_id, cycle, text_len);

            const size_t record_size = 3 + text_len;

            // 範囲内で、かつ指定idと一致するレコードだけを削除する
            const bool should_delete =
                record_id == id &&
                pos >= start_offset &&
                pos <= end_offset;

            if (!should_delete)
            {
                // 削除対象でなければ、必要に応じて左へ移動する
                if (write_pos != pos)
                    memmove(data + write_pos, data + pos, record_size);

                write_pos += record_size;
            }

            // posは詰め直し前の位置を使って次のレコードへ進める
            pos += record_size;
        }

        // 詰め直し後の有効データの末尾を保存する
        targetindex = write_pos;
    }

    /**
     * @brief   指定したidとcycleの最新レコードの本文を置き換える
     *
     * 同じidとcycleのレコードが複数ある場合は、最も新しいレコードを対象にする
     * 置換後に容量が足りなくなる場合は、置換対象を残したまま、
     * それ以外の最も古いレコードから削除して空きを作る
     *
     * @param id        置換対象の記録を持つid（friends_idまたはmy_id）
     * @param cycle     置換対象のcycle
     * @param text      新しい本文
     * @param text_size 新しい本文の長さ（バイト数'\0'は含めない）
     */
    void reappend(uint8_t id, uint8_t cycle, char *text, size_t text_size)
    {
        // 本文のサイズとポインターが正しいか確認する
        if (text_size > infra::text_size_max || (text_size > 0 && text == nullptr))
            return;

        // 破損した記録を誤って書き換えない
        if (!is_valid())
            return;

        const size_t new_record_size = 3 + text_size;

        // 新しいレコード1件だけでも配列に収まらなければ置換できない
        if (new_record_size > size)
            return;

        // 容量を確保する処理で本文が変化しないよう、先に退避する
        char text_copy[infra::text_size_max];

        if (text_size > 0)
            memcpy(text_copy, text, text_size);

        // 指定したidとcycleのうち、最も新しいレコードの位置を探す
        size_t target_offset = 0;
        bool found_target = false;

        for (size_t pos = 0; pos < targetindex;)
        {
            uint8_t record_id, record_cycle, text_len;
            read_header(
                data, size, targetindex, pos,
                record_id, record_cycle, text_len);

            if (record_id == id && record_cycle == cycle)
            {
                target_offset = pos;
                found_target = true;
            }

            pos += 3 + text_len;
        }

        // 対象のレコードが見つからなければ何も変更しない
        if (!found_target)
            return;

        // 置換前のレコードから、id・cycle・本文の長さを取得する
        uint8_t target_id, target_cycle, old_text_len;
        read_header(
            data, size, targetindex, target_offset,
            target_id, target_cycle, old_text_len);

        const size_t old_record_size = 3 + old_text_len;

        // 置換後に容量を超える間、対象以外の最も古いレコードを削除する
        while (targetindex - old_record_size + new_record_size > size)
        {
            size_t pos = 0;
            bool removed = false;

            while (pos < targetindex)
            {
                uint8_t record_id, record_cycle, text_len;
                read_header(
                    data, size, targetindex, pos,
                    record_id, record_cycle, text_len);

                const size_t record_size = 3 + text_len;

                if (pos != target_offset)
                {
                    if (!remove_record_at(pos))
                        return;

                    // 削除位置が対象より前なら、対象位置も左へ移動する
                    if (pos < target_offset)
                        target_offset -= record_size;

                    removed = true;
                    break;
                }

                pos += record_size;
            }

            // 対象以外のレコードがなく、容量を確保できない場合は終了する
            if (!removed)
                return;
        }

        // 置換対象の後ろにあるデータの開始位置と長さを求める
        const size_t tail_offset = target_offset + old_record_size;
        const size_t tail_size = targetindex - tail_offset;

        // 新旧の本文サイズに応じて、後続データを右または左へ移動する
        // memmoveは移動元と移動先が重なっていても安全に処理する
        memmove(
            data + target_offset + new_record_size,
            data + tail_offset,
            tail_size);

        // 置換対象のヘッダーと本文を書き込む
        data[target_offset] = static_cast<char>(target_id);
        data[target_offset + 1] = static_cast<char>(target_cycle);
        data[target_offset + 2] = static_cast<char>(text_size);

        if (text_size > 0)
            memcpy(data + target_offset + 3, text_copy, text_size);

        // レコード全体のサイズ差に合わせて有効データの末尾を更新する
        if (new_record_size >= old_record_size)
            targetindex += new_record_size - old_record_size;
        else
            targetindex -= old_record_size - new_record_size;
    }

    /**
     * @brief logを新しい順に取得する
     *
     * @param num       欲しいデータの番号（0番が最新）
     * @param id        データに含まれるid
     * @param cycle     データに含まれるcycle
     * @param text      本文の先頭を指すポインター
     * @param text_len  本文の長さ（バイト数）
     *
     * @return 取得できた場合はtrue
     */
    bool get(
        size_t num,
        uint8_t &id,
        uint8_t &cycle,
        const char *&text,
        uint8_t &text_len)
    {
        // 失敗時の出力値
        id = 0;
        cycle = 0;
        text = nullptr;
        text_len = 0;

        // ログが壊れている場合は取得しない
        if (!is_valid())
            return false;

        // レコード数を数える
        size_t record_count = 0;

        for (size_t pos = 0; pos < targetindex;)
        {
            uint8_t record_id, record_cycle, record_text_len;

            read_header(
                data, size, targetindex, pos,
                record_id, record_cycle, record_text_len);

            pos += 3 + record_text_len;
            ++record_count;
        }

        // numは0始まり。レコード数以上なら該当データなし
        if (num >= record_count)
            return false;

        // 配列は古い順なので、新しい順のnum番目を古い順の位置に変換
        const size_t target_index = record_count - 1 - num;

        // 対象レコードまで走査する
        size_t index = 0;

        for (size_t pos = 0; pos < targetindex;)
        {
            uint8_t record_id, record_cycle, record_text_len;

            read_header(
                data, size, targetindex, pos,
                record_id, record_cycle, record_text_len);

            if (index == target_index)
            {
                id = record_id;
                cycle = record_cycle;
                text_len = record_text_len;

                // 本文はヘッダー3バイトの直後
                text = data + pos + 3;
                return true;
            }

            pos += 3 + record_text_len;
            ++index;
        }

        return false;
    }
}