IPPhone
===

## 環境構築

このプログラムをコンパイルするために必要なものと、準備の有無の確認方法、インストール方法を列挙する。

*   `g++`: c++コンパイラの1つ
    *   Check: `g++ --version`でバージョン`7`以上の存在が示される
    *   Install:
        ```
        $> sudo apt install g++
        ```
*   `cmake`: `Makefile`を簡単に、移植性高く生成する
    *   Check: `cmake --version`でバージョン`3.10`以上の存在が示される
    *   Install:
        ```
        $> sudo apt install cmake
        ```
*   `make`: コンパイル時に実行するコマンドを`Makefile`書いておいて、簡単にできるようにする
    *   Check: `make --version`で存在が示される
    *   Install:
        ```
        $> sudo apt install build-essential
        ```
*   `pkg-config`: ライブラリを使う上での情報を取り出す。`cmake`の中などで用いる。
    *   Check: `pkg-config --version`で存在が示される
    *   Install:
        ```
        $> sudo apt install pkg-config
        ```
*   `boost`: c++の標準の次に一般的なライブラリ
    *   Check: 以下に述べる`cmake`の実行をする
    *   Install:
        ```
        $> sudo apt install libboost-all-dev
        $> sudo ln -s /usr/include/boost /usr/local/include/boost
        ```
*   `libpulse`: Linuxの音声入出力ライブラリ
    *   Check: `pkg-config`をインストールした上での`pkg-config libpulse --exists && echo 'ok'`で`ok`と表示される
    *   Install:
        ```
        $> sudo apt install libpulse-dev
        ```

また、このGit RepositoryをCloseするのに加え、SubmoduleもCloseする必要が有る。
```
$> git submodule update --init --recursive
```
を実行する。

### コンパイル手順

環境構築を終えた上で`I3`のディレクトリに移動して、次を実行する。

```
$> mkdir -p build
$> cd build
$> cmake ..
$> make -j $(nproc)
```

## ディレクトリ構成

```
I3
├── cereal
├── client
│   ├── CMakeLists.txt
│   ├── main.cpp
│   └── tcp_connect.hpp
├── CMakeLists.txt
├── common
│   ├── CMakeLists.txt
│   ├── endpoint.hpp
│   ├── params
│   │   ├── communication.hpp
│   │   └── sound.hpp
│   └── pulseaudio
│       ├── context.cpp
│       ├── context.hpp
│       ├── exception.hpp
│       ├── mainloop.cpp
│       ├── mainloop.hpp
│       ├── playback_stream.cpp
│       ├── playback_stream.hpp
│       ├── record_stream.cpp
│       ├── record_stream.hpp
│       ├── stream_base.cpp
│       └── stream_base.hpp
└── server
    ├── CMakeLists.txt
    ├── main.cpp
    └── tcp_accept.hpp
```

### `CMakeLists.txt`

先述の`cmake`に向けての設定を行っている。

### `cereal`

データのフォーマット周りを簡単にできるようにするライブラリ。

具体的に言うと、各データについてフォーマットを指定する関数が用意してあれば、`std::istream`や`std::ostream`とのバイナリ単位での入出力ができてしまう。

### `common`

サーバ・クライアント両方に関わるコードはここに置いた。

#### `pulseaudio`

*   `libpulse`のAPIはc言語で書かれているので、他のc++の書き方に比べると煩雑になってしまう。
*   非同期処理のための排他処理は、プログラムの粒度を高めて対応したい。
*   サーバもクライアントも再生周りは全く変わらない

これらの理由から、`common`の中に`libpulse`のラッパーを置いておいた。

#### `params`

音声のサンプリングパラメータや、通信データの基本サイズなど、サーバ・クライアントで共有できる/するべきパラメータをここに置いておいた。

#### `endpoint.hpp`

TCPでもUDPでも、通信に必要な情報は変わらず、IPアドレスとポートである。それを表す型を定義した。

### `server`/`client`

この2つは、はじめにTCPで接続を確立する時にやることが違う。
その部分だけ`tcp_accept.hpp`, `tcp_connect.hpp`に書いた。

#### `main.cpp`

`libpulse`側でやることは、

1.  非同期に登録した処理を実行していく`threaded_mainloop`を生成する
2.  音声周りのハードウェアとの接続を扱う`context`を生成・接続する
3.  録音・再生のデータの流れを扱う`stream`を各々生成する
4.  録音したデータが一定以上溜まった時の(送信する)処理を登録する
5.  録音・再生の`stream`を`context`に接続する
6.  (通信を介してデータを受け取ったら)再生バッファに適切にコピーする

通信(`boost::asio`)側でやることは、

1.  非同期に登録した処理を実行し、システムコールを担う`context`を生成する
2.  (録音データが溜まったら)送信する処理を非同期に登録する
3.  通信先からデータを受信したら再生バッファにコピーする処理を、登録しておく
