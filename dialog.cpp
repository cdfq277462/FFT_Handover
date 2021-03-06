#include "dialog.h"
#include "ui_dialog.h"

#include "mathtools.h"

#ifndef FFT_N
#define FFT_N  4096
#endif // FFT_N

#ifndef PI
#define PI      3.14159
//#define PI      3.1415926535
#endif // PI

struct compx {float real,imag;};                                        //定義一個復數結構
struct compx s[FFT_N];

float s_real[FFT_N], s_imag[FFT_N];

float x_ticks[FFT_N / 2] = {0};
struct compx EE(struct compx a,struct compx b)
{
    struct compx c;

    c.real=a.real*b.real-a.imag*b.imag;
    c.imag=a.real*b.imag+a.imag*b.real;

    return(c);
};


Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    ui->frame->hide();

    ui->tableWidget->setColumnCount(5);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QStringList labels;
    labels<<"Origin Data"<<"FFT"<<"Interval"<<"Ch"<<"Output";
    ui->tableWidget->setHorizontalHeaderLabels(labels);
    ui->tableWidget->verticalHeader()->setVisible(false);

    QDir myDir = QDir().currentPath();
    myDir.setFilter(QDir::Files);
    QFileInfoList filelist = myDir.entryInfoList();
    foreach(QFileInfo filename, filelist)
        if(filename.fileName().contains("data"))
        {
            //qDebug() << filename;
            ui->comboBox->addItem(filename.fileName());
        }
}

Dialog::~Dialog()
{
    delete ui;
}


void FFT(float *xin_real, float *xin_imag)
{
    int f , m, nv2, nm1, i, k, l, j = 0;
    //struct compx u,w,t;
    float t_real, t_imag;
    float u_real, u_imag;
    float w_real, w_imag;

    nv2 = FFT_N / 2;                   //變址運算，即把自然順序變成倒位序，采用雷德算法
    nm1 = FFT_N - 1;
    for(i = 0; i < nm1; i++)
    {
        if(i < j)                      //如果i<j,即進行變址
        {
            t_real = xin_real[j];
            xin_real[j] = xin_real[i];
            xin_real[i] = t_real;
        }
        k = nv2;                       //求j的下一個倒位序
        while( k <= j)                 //如果k<=j,表示j的最高位為1
        {
            j = j - k;                 //把最高位變成0
            k = k / 2;                 //k/2，比較次高位，依次類推，逐個比較，直到某個位為0
        }
        j = j + k;                     //把0改為1
    }

    {
        int le , lei, ip;                            //FFT運算核，使用蝶形運算完成FFT運算

        f = FFT_N;
        for(l = 1; (f=f/2)!=1; l++)                  //計算l的值，即計算蝶形級數
            ;
        //qDebug() << l;
        for( m = 1; m <= l; m++)                         // 控制蝶形結級數
        {                                        //m表示第m級蝶形，l為蝶形級總數l=log（2）N
            le = 2 << (m - 1);                            //le蝶形結距離，即第m級蝶形的蝶形結相距le點
            lei = le / 2;                               //同一蝶形結中參加運算的兩點的距離
            u_real = 1.0;                             //u為蝶形結運算系數，初始值為1
            u_imag = 0.0;
            w_real = cos(PI / lei);                     //w為系數商，即當前系數與前一個系數的商
            w_imag = -sin(PI / lei);
            for(j = 0;j <= lei - 1; j++)                   //控制計算不同種蝶形結，即計算系數不同的蝶形結
            {
                for(i = j; i <= FFT_N - 1; i = i + le)            //控制同一蝶形結運算，即計算系數相同蝶形結
                {
                    ip = i + lei;                           //i，ip分別表示參加蝶形運算的兩個節點
                    //t = EE(xin[ip], u);                    //蝶形運算，詳見公式

                    t_real = xin_real[ip] *u_real - xin_imag[ip] *u_imag;
                    t_imag = xin_real[ip] *u_imag + xin_imag[ip] *u_real;

                    xin_real[ip] = xin_real[i] - t_real;
                    xin_imag[ip] = xin_imag[i] - t_imag;
                    xin_real[i] = xin_real[i] + t_real;
                    xin_imag[i] = xin_imag[i] + t_imag;
                }
                //u = EE(u, w);                           //改變系數，進行下一個蝶形運算
                u_real = u_real *w_real - u_imag *w_imag;
                u_imag = u_real *w_imag + u_imag *w_real;
            }
        }
    }
}

QVector<double> Dialog::SPG(QVector<double> datainput)
{
    float mean_s = 0;				                                            //mean of s[].real
    float interval[56] = {0};
    float sn[FFT_N/2] = {0};
    //int chs = 5*log2(FFT_N/4);
    //qDebug() << chs;
    int chs;
    int f = FFT_N/4;
    // calculate chs's value, equal "chs = 5*log2(FFT_N/4)"
    for(chs = 1; (f=f/2)!=1; chs++)
        ;
    chs = chs *5;
    //qDebug() << chs;

    QVector<double> y(chs +1);
//set channel

    for(int i = 0; i < FFT_N; i++)											//給結構體賦值
    {
    //data input
        float tmp = datainput[i];
        s_real[i] = tmp;
        s_imag[i] = 0;//虛部為0
       // printf("%1f\t" , s[i].real);
    }
//means
    for(int i = 0; i < FFT_N; i++)
        mean_s = mean_s + (s_real[i] / FFT_N);

    for(int i = 0; i < FFT_N; i++)
        s_real[i] = (s_real[i] - mean_s);

//do fft
    FFT(s_real, s_imag);

    for(int i = 0; i < FFT_N/2; i++)
    {
        //s[i].real = sqrt(s[i].real * s[i].real + s[i].imag * s[i].imag) ;    //求變換後結果，存入復數的實部部分
        s_real[i] = sqrt(s_real[i] * s_real[i] + s_imag[i] * s_imag[i]) ;



        //x_axis degree
        float tp = (i + 1);
        x_ticks[FFT_N/2 -1 - i] = (FFT_N / tp);                          // length(x_ticks[i]) = FFT_N/2


        //inverse s[i] to sn[i]
        float tmp = s_real[i] / FFT_N *2;
        //qDebug() << tmp;

        sn[FFT_N/2 -1 - i] = tmp;
        //y[FFT_N/2 - i -1] = tmp;
    }
    QTableWidgetItem *item = new QTableWidgetItem();

    // use to display
    for(int i = 0; i < FFT_N/2; i++)
    {
        item = new QTableWidgetItem(QString::number(sn[i]));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 1, item);
    }

//set channel
    for(int i= 0; i <= chs; i++){                //set how many chs
        interval[i] = pow(2 , 0.2*(i+5));       //pow(x, y)用來求 x 的 y 次方
        //printf("%f\n", interval[i]);

        // use to display
        item = new QTableWidgetItem(QString::number(interval[i]));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 2, item);
        item = new QTableWidgetItem(QString::number(i +1));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 3, item);
    }

    int j = 0;
    for(int i= 0; i <= chs; i++)
    {
        float max = 0;
        //find max btw interval[i] from interval[i + 1]
        if(x_ticks[j] > interval[i])
            max = sn[j];
        else
            while(x_ticks[j] <= interval[i])
            {
                if(sn[j] > max)
                    max = sn[j];
                j++;
            }
        y[i] = max;
        //printf("%d\t", j);
        //printf("%1f\n", y[i]);

        // use to display
        item = new QTableWidgetItem(QString::number(y[i]));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 4, item);
    }
    return y;
}
void Dialog::on_pushButton_calculateSPG_clicked()
{
    ui->frame->hide();
    QStringList data = readData();
    QVector<double> data_value;
    QVector<double> SPG_key;

    //qDebug() << data;

    ui->tableWidget->setRowCount(data.length());

    int i = 0;

    // use to display
    QTableWidgetItem *item = new QTableWidgetItem();
    foreach (QString Num, data) {
        item = new QTableWidgetItem(Num);
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 0, item);

        i++;
        data_value.append(Num.toDouble());
    }
    data_value = SPG(data_value);
    //qDebug() << data_value;

}

QStringList Dialog::readData()
{
    QVector<double> SPG_data;

    QString Filename = QDir().currentPath() + "/" + ui->comboBox->currentText();
    QFile config(Filename);

    if(!config.open(QFile::ReadOnly | QFile::Text)){
        qDebug() << "Cannot initialize parameter!";
    }
    QTextStream in(&config);
    QStringList para;

    //index == 0 : read All data
    //index == 1 : read in line para

    para = in.readAll().split("\n");
    while(para.length() > FFT_N)
        para.removeLast();
    config.close();

    return para;
}

void Dialog::on_pushButton_clicked()
{
    ui->frame->show();

    QStringList data = readData();
    QVector<double> SPG_value;
    QVector<double> SPG_key;

    ui->tableWidget->setRowCount(data.length());

    int i = 0;
    QTableWidgetItem *item = new QTableWidgetItem();
    foreach (QString Num, data) {
        item = new QTableWidgetItem(Num);
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 0, item);
        i++;

        SPG_value.append(Num.toDouble());
    }
    SPG_value = SPG(SPG_value);

    //qDebug() << SPG_value.length();

    for(int j = 0; j < SPG_value.length(); j++)
    {
        //SPG_x.append(pow(2 , 0.2*(j+5)));
        SPG_key.append(j);
    }

    ui->widget->addGraph(0);
    //ui->widget_SPG->graph(0)->setData(SPG_x, SPG);

    QCPBars *SPG_Bar = new QCPBars(ui->widget->xAxis, ui->widget->yAxis);
    SPG_Bar->setData(SPG_key, SPG_value);
    ui->widget->xAxis->setRange(1, i);


    QSharedPointer<QCPAxisTickerText> TranToLoger(new QCPAxisTickerText);
    QVector<double> Doubleticks;
    QVector<QString> LogTicks;
    Doubleticks << log2(2)*5-5 << log2(5)*5-5 << log2(10)*5-5 << log2(20)*5-5 << log2(50)*5-5 << log2(100)*5-5 << log2(200)*5-5 << log2(500)*5-5 << log2(1000)*5-5 << log2(2000)*5-5 << log2(5000)*5-5;
    LogTicks << "2" << "5" << "10" << "20" << "50" << "100" << "200" << "500" << "1000" << "2000" << "5000";
    TranToLoger->addTicks(Doubleticks, LogTicks);
    ui->widget->xAxis->setTicker(TranToLoger);


    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    //logTicker->setSubTickCount(3);
    //logTicker->setTickCount(50);
    SPG_Bar->setStackingGap(1);
    //logTicker->setLogBase(10);
    //ui->widget_SPG->xAxis->setTicker(logTicker);
    //ui->widget_SPG->xAxis->setScaleType(QCPAxis::stLogarithmic);

    //ui->widget_SPG->xAxis->setNumberFormat("gb");

    //SPG_Bar->setWidthType(QCPBars::wtAbsolute);
    SPG_Bar->setWidth(1);

    //ui->widget_SPG->axisRect()->setupFullAxesBox();

    ui->widget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->widget->rescaleAxes();
    ui->widget->replot();
    SPG_value.clear();
    SPG_key.clear();
    SPG_Bar->data()->clear();
}
