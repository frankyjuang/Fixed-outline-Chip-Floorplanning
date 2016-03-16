1.組員: B02901024鄭佳維、B02901026莊佾霖
2.使用之程式語言: C++
3.使用之編譯器: g++ 4.9.3
4.檔案壓縮方式: .zip
5.各檔案說明:
   EDA Project-Presentation.pptx :18周上台報告使用的投影片
   EDA Project-Proposal.pdf :15周所繳交的proposal
   EDA Project-Report.pdf   :本次project的書面報告
   btree.h       :使用的資料結構之.h檔
   focf.cpp      :這次所需fixed outline floorplanning的主程式
   focf.exe      :這次所需fixed outline floorplanning的執行檔
   makefile	 :執行make來編譯檔案即可
                   (因為有使用到C++11的function)
   README.txt   :本介紹
6.執行方式說明
   如同作業要求敘述的command-line
   [executable file name] [αvalue] [input.block name] [input.net name] [output file name]
7.執行結果說明
   應與sample output一致
8.特殊使用
   最上方Define處
   可以改T0:initial temperature
          P:trials per reduction of temperature
          r:temperature reduction rate
         NUM_TREE:一次跑幾棵樹
    我們預設的是在十分鐘內可以跑完所有benchmark的版本
    助教可以根據需求對上述四個參數進行調整