#### ライブラリーインポート ####
import sensor, image, time, math, gc

#### 初期設定 ####
sensor.reset(dual_buff=True)
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time=250)
sensor.set_contrast(-3)
sensor.set_brightness(-3)
sensor.set_saturation(0)
sensor.skip_frames(time=250)
sensor.set_auto_gain(False, gain_db=10, gain_db_ceiling=16)
sensor.set_auto_exposure(False, exposure_us=3000)

cam_wz = 11.5                       # カメラの高さ[cm]
px_max = sensor.width() / 2         # 横幅の半分[pixcel]
pz_max = sensor.height() / 2        # 縦幅の半分[pixcel]

alpha = math.radians(80)                                        # 対角視野角[rad]
pf = math.sqrt(px_max ** 2 + pz_max ** 2) / math.tan(alpha / 2) # 焦点距離[pixcel]

beta = math.radians(0)                                          # カメラの傾き[rad]

#### 色データ ####
default_LABdata = (100, 0, 127, -128, 127, -128)
ball_LABdata = (21, 82, 18, 74, 33, 78)

#### 関数 ####
def convert_to_pixcel_x_z_plane(x, y):  # ピクセルの座標平面上の位置(px,pz)に変換する関数
    _px = x - px_max
    _pz = - cam_wz * math.tan(beta) + (-y + pz_max) # カメラの傾きを考慮しピクセル座標を変更する
    return _px, _pz

def convert_to_world_x_y_z_plane(b):    # 現実の座標平面上の位置(x,y,z)に変換する関数
    # ピクセルの座標平面上の位置(px,pz)に変換する
    # ブロブの底の座標（地面と触れている部分）にする
    b_x_centor = b.cx()
    b_y_bottom = b.cy() + b.h() / 2
    px, pz = convert_to_pixcel_x_z_plane(b_x_centor, b_y_bottom)

    if(pz >= -1):
        return 0, 0, 999

    _wz = 0
    _wy = - pf * cam_wz / pz
    _wx = - px * cam_wz / pz
    return _wx, _wy, _wz

def sort_img(blobs):                    # imgをピクセル数が多い順にソートする関数
    if blobs:
        # key           :   関数を入れる
        # lambda v1: v2 :   即席の関数(v1が引数,v2が戻り値)
        # reverse=True  :   降順に並べる（デフォルトが昇順）
        blobs.sort(key=lambda b: b.pixels(), reverse=True)
    return blobs

#### メインループ ####
while(True):
    img = sensor.snapshot()         # カメラからの映像を撮影する

    # ボールのブロブを検出しピクセル数が多い順にソート
    # pixels_threshold  :   ピクセル数の最小値
    # merge=True        :   ある程度近くにあるブロブを一緒にする
    ball_blobs = img.find_blobs([ball_LABdata], pixels_threshold=10, merge=True)
    ball_blobs = sort_img(ball_blobs)

    # ボールの位置を現実の座標平面上の位置(x,y)に変換する
    if(ball_blobs):
        img.draw_rectangle(ball_blobs[0].rect(), color=(0,0,0))
        wx, wy, wz = convert_to_world_x_y_z_plane(ball_blobs[0])
        dis = math.sqrt(wx ** 2 + wy ** 2)
        deg = math.degrees(math.atan2(wx,wy))
        print("(" + str(wx) + ", " + str(wy) + ", " + str(wz) + ") deg:" + str(deg) + " dis:" + str(dis))
    else:
        print("not found")

    gc.collect()                    # 溜まった不要なメモリを強制的に掃除する
