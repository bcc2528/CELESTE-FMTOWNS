# CELESTE-FMTOWNS
仮想ゲームマシンエンジン「PICO-8」で配信されているフリーゲーム「CELESTE Classic」をFM TOWNSに移植したものです。FM TOWNSエミュレータ「津軽」「うんづ」及びFM TOWNS MARTY実機で動作確認を行っています。

ゲーム自体はほぼ移植できていますが、グラフィック・サウンドの仕様の違いから微妙な差異も存在します。

・TOWNS版は全グラフィックをスプライトで表示、30fpsを維持するため448枚までに制限しているため、スプライト表示数の多いシーンだと背景スクロールが一部欠ける。

・ダッシュ動作などで画面が揺れる際、画面右端が一部乱れる。

・頂上で画面中央に移動するたびに黒帯が画面を覆う表示はカット。

・PSG音源風のサウンドをFM音源で再現しているため音色が違う




～～～～～～～～～～～～～～～～～～操作方法～～～～～～～～～～～～～～～～～～

-パッド上下左右・・・移動

-パッドAボタン・・・ジャンプ

-パッドBボタン・・・ダッシュ(空中可。連続1回までだが中盤から2回まで可能となる)

-パッドRUNボタン・・・一時停止

-一時停止中にパッドSELECTボタン・・・タイトル画面に戻る

キーボードのどれかのキー・・・ゲーム終了




～～～～～～～～～～～～～～～～～ファイル構成～～～～～～～～～～～～～～～～～

CELESTE.DOC---このファイル

CELESTE.EXP---ゲーム実行ファイル

CELESTE.ICN---TownsOS登録用アイコンファイル

CELESTE.CTB---スプライトパレットデータ

CELESTE.PAT---スプライトパターンデータ

CELESTE.MAP---マップデータ

*.MSV---MSV形式音楽データ

PICO8.FMB---FM音源音色データ

SRCフォルダ---High-C及びNASM用のソースコード、MSV曲のコンパイル前MMLファイル収録




～～～～～～～～～～～～～現バージョンV1.1 L10での問題点～～～～～～～～～～～～

・FM TOWNS実機上だと髪の毛の表示位置がズレていたり雪のドットが全て大きい

・386機などではスプライトのチラツキが発生する箇所がある、最悪フリーズする

・浮遊するフルーツの羽・風船の糸のアニメーションパターン数が一つ表示されない

・連続ジャンプが可能となるオーブが出現する大箱の演出が簡素

・使用しているサウンドドライバ「MSVライブラリ V2.00」の問題か、稀にBGMテンポがおかしくなったり、効果音の多重出力ができない

  (MSVライブラリ最終版のV2.08をお持ちの方、お知らせください)
  
  


～～～～～～～～～～～～～～～～～～その他～～～～～～～～～～～～～～～～～～～

スプライトパターン・スプライトパレットの制作にはフリコレ10収録「FIGHT! V1.03A」(川嶋氏作)を使用させたいただきました。


FM音源音色の制作には「J-SOUND 1.03K」(HONESEN氏作)を使用させたいただきました。


サウンドドライバ使用及びMML制作には「MSVライブラリ for High-C V2.00」(僧侶の天使氏作)を使用させたいただきました。



～～～～～～～～～～～～～～～～～～著作～～～～～～～～～～～～～～～～～～～～

このゲームは「CELESTE Classic」をFM TOWNS向けに移植したものです。

ゲームに関する著作権は原作者であるMaddy Thorson氏及びNoel Berry氏に帰属します。

オリジナル版リンク

http://lexaloffle.com/bbs/?tid=2145
