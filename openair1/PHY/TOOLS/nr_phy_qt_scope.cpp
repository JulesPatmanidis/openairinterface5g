#include <QApplication>
#include <QtWidgets>
#include <QPainter>
#include <QtGui>
#include "nr_phy_qt_scope.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <cassert>
#include <cmath>
#include <QtCharts>
#include <qchart.h>
#include <QValueAxis>
#include <stdio.h>
#include <dirent.h>
#include<QLineEdit>
#include<QFormLayout>
#include<QIntValidator>
#include<stdlib.h>

extern "C" {
#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include <openair1/PHY/CODING/nrPolar_tools/nr_polar_defs.h>
}

typedef struct complex16 scopeSample_t;
#define ScaleZone 4;
#define SquaredNorm(VaR) ((VaR).r*(VaR).r+(VaR).i*(VaR).i)

float Limits_KPI_gNB[4][2]={ // {lower Limit, Upper Limit}
  {0.02, 0.8},    // UL BLER
  {0.2, 10},      // UL Throughput in Mbs
  {0.02, 0.8},    // DL BLER
  {0.2, 10}       // DL Throughput in Mbs
};

float Limits_KPI_ue[2][2]={ // {lower Limit, Upper Limit}
  {0.02, 0.8},    // DL BLER
  {0.2, 10}       // Throughput in Mbs
};

typedef struct {
  int dataSize;
  int elementSz;
  int colSz;
  int lineSz;
} scopeGraphData_t;

KPIListSelect::KPIListSelect(QWidget *parent) : QComboBox(parent)
{
  this->addItem("I/Q PBCH", 0);
  this->addItem("LLR PBCH", 1);
  this->addItem("I/Q PDSCH", 2);
  this->addItem("LLR PDSCH", 3);
  this->addItem("I/Q PDCCH", 4);
  this->addItem("LLR PDCCH", 5);
  this->addItem("RX Signal-Time", 6);
  this->addItem("Channel Response", 7);
  this->addItem("DL BLER", 8);
  this->addItem("Throughput", 9);
  this->addItem("DL MCS", 10);
  this->addItem("Nof Sched. RBs", 11);
  this->addItem("Freq. Offset/Time Adv.", 12);
  this->addItem("Configs", 13);
}
KPIListSelect::~KPIListSelect()
{
}

KPIListSelectgNB::KPIListSelectgNB(QWidget *parent) : QComboBox(parent)
{
  this->addItem("I/Q PUSCH", 0);
  this->addItem("LLR PUSCH", 1);
  this->addItem("Channel Response", 2);
  this->addItem("UL BLER", 3);
  this->addItem("DL BLER", 4);
  this->addItem("DL MCS", 5);
  this->addItem("UL MCS", 6);
  this->addItem("UL Throughput", 7);
  this->addItem("DL Throughput", 8);
  this->addItem("Nof Sched. RBs", 9);
  this->addItem("UL SNR", 10);
  this->addItem("DL SNR (CQI)", 11);
  this->addItem("UL Retrans.", 12);
  this->addItem("DL Retrans.", 13);
  this->addItem("Configs", 14);
  this->addItem("RX Signal-Time", 15);
}
KPIListSelectgNB::~KPIListSelectgNB()
{
}

configBoxgNB::~configBoxgNB()
{
}

configBoxgNB::configBoxgNB(QWidget *parent, int configIdx) : QLineEdit(parent)
{
  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &configBoxgNB::readText);
  timer->start(5000);

  this->configIdx = configIdx;
  this->setText("-10");
}

void configBoxgNB::readText()
{
  QString text_e1 = this->text();
  QByteArray ba = text_e1.toLocal8Bit();
  char *c_str2 = ba.data();

  if (this->configIdx == 0)
  {
    Limits_KPI_gNB[0][0] =  atof(c_str2);
  }
  else if (this->configIdx == 1)
  {
    Limits_KPI_gNB[0][1] = atof(c_str2);
  }
  else if (this->configIdx == 2)
  {
    Limits_KPI_gNB[1][0] = atof(c_str2);
  }
  else if (this->configIdx == 3)
  {
    Limits_KPI_gNB[1][1] = atof(c_str2);
  }
  else if (this->configIdx == 4)
  {
    Limits_KPI_gNB[2][0] =  atof(c_str2);
  }
  else if (this->configIdx == 5)
  {
    Limits_KPI_gNB[2][1] = atof(c_str2);
  }
  else if (this->configIdx == 6)
  {
    Limits_KPI_gNB[3][0] = atof(c_str2);
  }
  else if (this->configIdx == 7)
  {
    Limits_KPI_gNB[3][1] = atof(c_str2);
  }
}

configBoxgUE::~configBoxgUE()
{
}

configBoxgUE::configBoxgUE(QWidget *parent, int configIdx) : QLineEdit(parent)
{
  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &configBoxgUE::readText);
  timer->start(5000);

  this->configIdx = configIdx;
  this->setText("-10");
}

void configBoxgUE::readText()
{
  QString text_e1 = this->text();
  QByteArray ba = text_e1.toLocal8Bit();
  char *c_str2 = ba.data();

  if (this->configIdx == 0)
  {
    Limits_KPI_ue[0][0] =  atof(c_str2);
  }
  else if (this->configIdx == 1)
  {
    Limits_KPI_ue[0][1] = atof(c_str2);
  }
  else if (this->configIdx == 2)
  {
    Limits_KPI_ue[1][0] = atof(c_str2);
  }
  else if (this->configIdx == 3)
  {
    Limits_KPI_ue[1][1] = atof(c_str2);
  }
}

PainterWidgetgNB::PainterWidgetgNB(QComboBox *parent, scopeData_t *p)
{
    timer = new QTimer(this);
    timerRetrans = new QTimer(this);
    this->chartHight = 300;
    this->chartWidth = 300;
    this->pix = new QPixmap(this->chartWidth,this->chartHight);
    this->pix->fill(QColor(240,240,240));
    this->parentWindow = parent;

    QScatterSeries *series = new QScatterSeries();
    this->chart = new QChart();
    this->chart->addSeries(series);
    this->chartView = new QChartView(this->chart, this);
    this->chartView->resize(this->chartWidth, this->chartHight);
    this->resize(this->chartWidth, this->chartHight);
    this->isOpenGLUsed = false; 
    this->chartView->hide();
    this->axisX = new QValueAxis;
    this->axisY = new QValueAxis;
    this->chart->addAxis(this->axisX, Qt::AlignBottom);
    this->chart->addAxis(this->axisY, Qt::AlignLeft);


    this->previousScalingMin = 200000;   // large enough to be overwritten in the first check
    this->previousScalingMax = -200000;  // small enough to be overwritten in the first check

    // settings for waterfall graph
    this->iteration = 0;
    this->waterFallh = this->chartHight/2 - 15;
    this->waterFallAvg= (double*) malloc(sizeof(*this->waterFallAvg) * this->waterFallh);


    this->p = p;
    this->nb_UEs = 1;
    this->current_instance = 0;

    this->indexToPlot = this->parentWindow->currentIndex();
    this->previousIndex = this->parentWindow->currentIndex();

    // UL BLER
    this->ULBLER.plot_idx = 0;
    resetKPIPlot(&this->ULBLER);

    // UL MCS
    this->ULMCS.plot_idx = 0;
    resetKPIPlot(&this->ULMCS);
    resetKPIValues(&this->ULMCS);

    // DL BLER
    this->DLBLER.plot_idx = 0;
    resetKPIPlot(&this->DLBLER);

    // DL MCS
    this->DLMCS.plot_idx = 0;
    resetKPIPlot(&this->DLMCS);
    resetKPIValues(&this->DLMCS);

    // UL Throughput
    this->ULThrou.plot_idx = 0;
    resetKPIPlot(&this->ULThrou);
    resetKPIValues(&this->ULThrou);

    // DL Throughput
    this->DLThrou.plot_idx = 0;
    resetKPIPlot(&this->DLThrou);
    resetKPIValues(&this->DLThrou);

    // nof scheduled RBs
    this->nofRBs.plot_idx = 0;
    resetKPIPlot(&this->nofRBs);
    resetKPIValues(&this->nofRBs);

    // UL SNR
    this->ULSNR.plot_idx = 0;
    resetKPIPlot(&this->ULSNR);
    resetKPIValues(&this->ULSNR);

    // DL SNR
    this->DLSNR.plot_idx = 0;
    resetKPIPlot(&this->DLSNR);
    resetKPIValues(&this->DLSNR);

    // UL Retrans
    this->ULRetrans[0].plot_idx = 0;
    for (int i=0; i<4; i++)
    {
      resetKPIPlot(&this->ULRetrans[i]);
      resetKPIValues(&this->ULRetrans[i]);
    }
    this->ULRetrans[0].series->setName("R. 1");
    this->ULRetrans[1].series->setName("R. 2");
    this->ULRetrans[2].series->setName("R. 3");
    this->ULRetrans[3].series->setName("R. 4");

    // DL Retrans
    this->DLRetrans[0].plot_idx = 0;
    for (int i=0; i<4; i++)
    {
      resetKPIPlot(&this->DLRetrans[i]);
      resetKPIValues(&this->DLRetrans[i]);
    }
    this->DLRetrans[0].series->setName("R. 1");
    this->DLRetrans[1].series->setName("R. 2");
    this->DLRetrans[2].series->setName("R. 3");
    this->DLRetrans[3].series->setName("R. 4");

    makeConnections();
}

void PainterWidgetgNB::resetKPIPlot(KPI_elements *inputStruct)
{
  inputStruct->series = new QLineSeries();
  inputStruct->series->setColor(QColor(0,0,0));

  inputStruct->seriesMin = new QLineSeries();
  inputStruct->seriesMin->setColor(QColor(255,0,0));
  inputStruct->seriesMin->setName("Min.");

  inputStruct->seriesMax = new QLineSeries();
  inputStruct->seriesMax->setColor(QColor(0,255,0));
  inputStruct->seriesMax->setName("Max.");

  inputStruct->seriesAvg = new QLineSeries();
  inputStruct->seriesAvg->setColor(QColor(0,0,255));
  inputStruct->seriesAvg->setName("Avg.");
}

void PainterWidgetgNB::resetKPIValues(KPI_elements *inputStruct)
{
  inputStruct->max_value = 0.0;
  inputStruct->min_value = 0.0;
  inputStruct->avg_value = 0.0;
  inputStruct->avg_idx = 0;
  inputStruct->nof_retrans = 0;
}

void PainterWidgetgNB::resizeEvent(QResizeEvent *event)
{
  if ((width() != this->chartWidth) || (height() != this->chartHight))
  {
    this->chartHight = height();
    this->chartWidth = width();

    // reset for waterfall plot
    if (this->indexToPlot == 15)
    {
      QPixmap *newPix = new QPixmap(this->chartWidth,this->chartHight);
      this->pix = newPix;
      this->pix->fill(QColor(240,240,240));
      this->iteration = 0;
      this->waterFallh = this->chartHight/2 - 15;
      for (int i=0; i< this->waterFallh; i++)
        this->waterFallAvg[i]=0;
    }
    update();
  }
  QWidget::resizeEvent(event);
}

void PainterWidgetgNB::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.drawPixmap( (this->width()-this->pix->width())/2,
  (this->height()-this->pix->height())/2, *this->pix); // paint pixmap on widget

  this->indexToPlot = this->parentWindow->currentIndex();

  makeConnections();
}

void PainterWidgetgNB::makeConnections()
{
  this->indexToPlot = this->parentWindow->currentIndex();
  disconnect(timer, nullptr, nullptr, nullptr);

  if ((this->indexToPlot != this->previousIndex) && (this->indexToPlot == 15))   // reset settings
  {
    this->pix->fill(QColor(240,240,240));
    this->iteration = 0;
    for (int i=0; i< this->waterFallh; i++)
      this->waterFallAvg[i]=0;
  }

  if (this->indexToPlot == 0)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_PuschIQ);
  }
  else if (this->indexToPlot == 1)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_PuschLLR);
  }
  else if (this->indexToPlot == 2)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_ChannelResponse);
  }
  else if (this->indexToPlot == 3)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_UL_BLER);
  }
  else if (this->indexToPlot == 4)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_DL_BLER);
  }
  else if (this->indexToPlot == 5)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_DL_MCS);
  }
  else if (this->indexToPlot == 6)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_UL_MCS);
  }
  else if (this->indexToPlot == 7)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_UL_Throu);
  }
  else if (this->indexToPlot == 8)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_DL_Throu);
  }
  else if (this->indexToPlot == 9)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_Nof_RBs);
  }
  else if (this->indexToPlot == 10)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_UL_SNR);
  }
  else if (this->indexToPlot == 11)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_DL_SNR);
  }
  else if (this->indexToPlot == 12)
  {
    connect(timerRetrans, &QTimer::timeout, this, &PainterWidgetgNB::KPI_UL_Retrans);
  }
  else if (this->indexToPlot == 13)
  {
    connect(timerRetrans, &QTimer::timeout, this, &PainterWidgetgNB::KPI_DL_Retrans);
  }
  else if ((this->indexToPlot == 14) && (this->current_instance == 0))
  {
    KPI_configurations();
  }
  else if (this->indexToPlot == 15)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidgetgNB::KPI_waterFall);
  }

  timer->start(100);
  timerRetrans->start(1000);
}

void PainterWidgetgNB::KPI_waterFall()
{
  this->chartView->hide();
  QPainter PixPainter(this->pix);
  PixPainter.translate(0, this->pix->height()/4);

  scopeSample_t *values = (scopeSample_t *) this->p->ru->common.rxdata[0];
  NR_DL_FRAME_PARMS *frame_parms=&this->p->gNB->frame_parms;
  const int datasize = frame_parms->samples_per_frame;

  if (values == NULL)
    return;

  this->waterFallh = this->chartHight/2 - 15;
  const int samplesPerPixel = datasize/this->chartWidth;
  int displayPart = this->waterFallh - ScaleZone;

  int row = this->iteration%displayPart;
  double avg=0;

  for (int i=0; i < displayPart; i++)
    avg+=this->waterFallAvg[i];

  avg/=displayPart;
  this->waterFallAvg[row]=0;

  for (int pix=0; pix<this->chartWidth; pix++)
  {
    scopeSample_t *end=values+(pix+1)*samplesPerPixel;
    end-=2;

    double val=0;

    for (scopeSample_t *s=values+(pix)*samplesPerPixel; s <end; s++)
      val += SquaredNorm(*s);

    val/=samplesPerPixel;
    this->waterFallAvg[row]+=val/this->chartWidth;

    if (val > avg*2 )
    {
      QColor IQColor(0,0,255);
      PixPainter.setPen(IQColor);
    }

    if (val > avg*10 ){
      QColor IQColor(0,255,0);
      PixPainter.setPen(IQColor);
    }

    if (val > avg*100 ){
      QColor IQColor(255,255,0);
      PixPainter.setPen(IQColor);
    }

    PixPainter.drawEllipse( QPoint(pix, this->iteration%displayPart), 2, 2 );
  }

  // Plot vertical Lines
  const float verticalSpacing = (float)this->chartWidth / (float)frame_parms->slots_per_frame;

  float startPointUp = -5;
  float startPointDown = startPointUp + this->waterFallh;
  for (uint16_t i = 0; i < frame_parms->slots_per_frame; i++)
  {
    float lineX = (float)i * verticalSpacing;
    QColor IQColor(0,0,0);
    PixPainter.setPen(IQColor);
    PixPainter.drawLine(QPoint(lineX, startPointUp), QPoint(lineX, startPointUp + 5));
    PixPainter.drawLine(QPoint(lineX, startPointDown), QPoint(lineX, startPointDown + 5));
  }

  this->iteration++;
  this->previousIndex = 15;
  this->isOpenGLUsed = false;
  repaint();

}
void PainterWidgetgNB::KPI_configurations()
{
  QWidget *window_1 = new QWidget();
  window_1->resize(300, 300);
  window_1->setWindowTitle("gNB Configs");
  configBoxgNB * configItem1 = new configBoxgNB(window_1, 0);
  configBoxgNB * configItem2 = new configBoxgNB(window_1, 1);
  configBoxgNB * configItem3 = new configBoxgNB(window_1, 2);
  configBoxgNB * configItem4 = new configBoxgNB(window_1, 3);
  configBoxgNB * configItem5 = new configBoxgNB(window_1, 4);
  configBoxgNB * configItem6 = new configBoxgNB(window_1, 5);
  configBoxgNB * configItem7 = new configBoxgNB(window_1, 6);
  configBoxgNB * configItem8 = new configBoxgNB(window_1, 7);

  QFormLayout *flo = new QFormLayout();
  flo->addRow("U-BLER lower",configItem1);
  flo->addRow("U-BLER upper",configItem2);
  flo->addRow("U-Throughput lower[Mbs]",configItem3);
  flo->addRow("U-Throughput upper[Mbs]",configItem4);
  flo->addRow("D-BLER lower",configItem5);
  flo->addRow("D-BLER upper",configItem6);
  flo->addRow("D-Throughput lower[Mbs]",configItem7);
  flo->addRow("D-Throughput upper[Mbs]",configItem8);

  window_1->setLayout(flo);
  window_1->show();
  this->current_instance++;
}

void PainterWidgetgNB::KPI_DL_Retrans()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];

  if ((this->DLRetrans[0].plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->DLRetrans[0].plot_idx = 0;
    this->chart->removeAllSeries();
    for (int i=0; i<4; i++)
    {
      resetKPIPlot(&this->DLRetrans[i]);
      resetKPIValues(&this->DLRetrans[i]);
    }
    this->DLRetrans[0].series->setName("R. 1");
    this->DLRetrans[1].series->setName("R. 2");
    this->DLRetrans[2].series->setName("R. 3");
    this->DLRetrans[3].series->setName("R. 4");
  }

  float Xpaint = this->DLRetrans[0].plot_idx;
  float Ypaint[4];
  uint64_t nrRetrans;
  for (int i=0; i<4;i++){
    nrRetrans = targetUE->mac_stats.dl.rounds[i] - this->DLRetrans[i].nof_retrans;
    Ypaint[i] = (float)nrRetrans;
    this->DLRetrans[i].nof_retrans = targetUE->mac_stats.dl.rounds[i];
  }

  if (this->DLRetrans[0].plot_idx != 0)
  {
    this->DLRetrans[0].max_value = std::max(this->DLRetrans[0].max_value, Ypaint[0]);
    for (int i=0; i<4;i++){
      this->DLRetrans[i].series->append(Xpaint, Ypaint[i]);
    }
  }

  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->setVisible(true);
  this->chart->legend()->setAlignment(Qt::AlignBottom);

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, this->DLRetrans[0].max_value + 5);
  this->axisY->setTitleText("Nof Retrans.");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if (this->DLRetrans[0].plot_idx == 1)
  {
    for (int i=0; i<4;i++){
      this->chart->addSeries(this->DLRetrans[i].series);
    }
  }
  for (int i=0; i<4;i++){
    this->DLRetrans[i].series->attachAxis(this->axisX);
    this->DLRetrans[i].series->attachAxis(this->axisY);
  }

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->DLRetrans[0].plot_idx++;
  this->previousIndex = 13;
  makeConnections();
}


void PainterWidgetgNB::KPI_UL_Retrans()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];

  if ((this->ULRetrans[0].plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->ULRetrans[0].plot_idx = 0;
    this->chart->removeAllSeries();
    for (int i=0; i<4; i++)
    {
      resetKPIPlot(&this->ULRetrans[i]);
      resetKPIValues(&this->ULRetrans[i]);
    }
    this->ULRetrans[0].series->setName("R. 1");
    this->ULRetrans[1].series->setName("R. 2");
    this->ULRetrans[2].series->setName("R. 3");
    this->ULRetrans[3].series->setName("R. 4");
  }

  float Xpaint = this->ULRetrans[0].plot_idx;
  float Ypaint[4];
  uint64_t nrRetrans;
  for (int i=0; i<4;i++){
    nrRetrans = targetUE->mac_stats.ul.rounds[i] - this->ULRetrans[i].nof_retrans;
    Ypaint[i] = (float)nrRetrans;
    this->ULRetrans[i].nof_retrans = targetUE->mac_stats.ul.rounds[i];
  }

  if (this->ULRetrans[0].plot_idx != 0)
  {
    this->ULRetrans[0].max_value = std::max(this->ULRetrans[0].max_value, Ypaint[0]);
    for (int i=0; i<4;i++){
      this->ULRetrans[i].series->append(Xpaint, Ypaint[i]);
    }
  }

  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->setVisible(true);
  this->chart->legend()->setAlignment(Qt::AlignBottom);

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, this->ULRetrans[0].max_value + 5);
  this->axisY->setTitleText("Nof Retrans.");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if (this->ULRetrans[0].plot_idx == 1)
  {
    for (int i=0; i<4;i++){
      this->chart->addSeries(this->ULRetrans[i].series);
    }
  }
  for (int i=0; i<4;i++){
    this->ULRetrans[i].series->attachAxis(this->axisX);
    this->ULRetrans[i].series->attachAxis(this->axisY);
  }

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->ULRetrans[0].plot_idx++;
  this->previousIndex = 12;
  makeConnections();
}


void PainterWidgetgNB::KPI_DL_SNR()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &targetUE->UE_sched_ctrl;

  if ((this->DLSNR.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->DLSNR.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->DLSNR);
  }

  float Xpaint, Ypaint;
  Xpaint = this->DLSNR.plot_idx;
  Ypaint = (float)sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.wb_cqi_1tb;

  this->DLSNR.max_value = std::max(this->DLSNR.max_value, Ypaint);
  this->DLSNR.series->append(Xpaint, Ypaint);

  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->hide();

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, this->DLSNR.max_value + 2);
  this->axisY->setTitleText("DL SNR (CQI)");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if(this->DLSNR.plot_idx == 0){
    this->chart->addSeries(this->DLSNR.series);
    this->chartView->setChart(this->chart);
  }
  this->DLSNR.series->attachAxis(this->axisX);
  this->DLSNR.series->attachAxis(this->axisY);

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->DLSNR.plot_idx++;
  this->previousIndex = 11;
  makeConnections();
}


void PainterWidgetgNB::KPI_UL_SNR()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &targetUE->UE_sched_ctrl;

  if ((this->ULSNR.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->ULSNR.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->ULSNR);
  }

  float Xpaint, Ypaint;
  Xpaint = this->ULSNR.plot_idx;
  Ypaint = (float)sched_ctrl->pusch_snrx10/10.0;

  this->ULSNR.max_value = std::max(this->ULSNR.max_value, Ypaint);
  this->ULSNR.series->append(Xpaint, Ypaint);

  this->chart->legend()->hide();
  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, 1.2*this->ULSNR.max_value);
  this->axisY->setTitleText("PUSCH SNR dB");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if (this->ULSNR.plot_idx == 0){
    this->chart->addSeries(this->ULSNR.series);
    this->chartView->setChart(this->chart);
  }
  this->ULSNR.series->attachAxis(this->axisX);
  this->ULSNR.series->attachAxis(this->axisY);

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->ULSNR.plot_idx++;
  this->previousIndex = 10;
  makeConnections();
}


void PainterWidgetgNB::KPI_Nof_RBs()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &targetUE->UE_sched_ctrl;


  NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
  int current_harq_pid = (int)sched_pdsch->dl_harq_pid;

  uint16_t rbSize = (uint16_t)sched_ctrl->harq_processes[current_harq_pid].sched_pdsch.rbSize;

  if ((this->nofRBs.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->nofRBs.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->nofRBs);
  }

  float Xpaint, Ypaint;
  Xpaint = this->nofRBs.plot_idx;
  Ypaint = (float)rbSize;

  this->nofRBs.max_value = std::max(this->nofRBs.max_value, Ypaint);
  this->nofRBs.series->append(Xpaint, Ypaint);

  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->hide();

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, this->nofRBs.max_value + 10);
  this->axisY->setTitleText("Nof Scheduled RBs");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if(this->nofRBs.plot_idx == 0){
    this->chart->addSeries(this->nofRBs.series);
    this->chartView->setChart(this->chart);
  }
  this->nofRBs.series->attachAxis(this->axisX);
  this->nofRBs.series->attachAxis(this->axisY);

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->nofRBs.plot_idx++;
  this->previousIndex = 9;
  makeConnections();
}


void PainterWidgetgNB::KPI_DL_Throu()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &targetUE->UE_sched_ctrl;
  uint32_t blockSize = (uint32_t)targetUE->mac_stats.dl.current_bytes;
  float bler_dl = (float)sched_ctrl->dl_bler_stats.bler;

  NR_DL_FRAME_PARMS *frame_parms = &this->p->gNB->frame_parms;
  uint16_t slots_per_frame = frame_parms->slots_per_frame;
  float slotDuration = 10.0/(float)slots_per_frame;      // slot duration in msec

  double blerTerm = 1.0 - (double)bler_dl;
  double blockSizeBits = (double)(blockSize << 3);

  double ThrouputKBitSec = blerTerm * blockSizeBits / (double)slotDuration;

  if ((this->DLThrou.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->DLThrou.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->DLThrou);
  }

  float Xpaint, Ypaint;
  Xpaint = this->DLThrou.plot_idx;
  Ypaint = (float)(ThrouputKBitSec/1000);    // Throughput in MBit/sec

  this->DLThrou.max_value = std::max(this->DLThrou.max_value, Ypaint);
  this->DLThrou.series->append(Xpaint, Ypaint);

  QLineSeries *series_LowLim = new QLineSeries();
  series_LowLim->append(0, Limits_KPI_gNB[3][0]);
  series_LowLim->append(this->chartWidth, Limits_KPI_gNB[3][0]);
  series_LowLim->setColor(QColor(0, 255, 0));

  QLineSeries *series_UppLim = new QLineSeries();
  series_UppLim->append(0, Limits_KPI_gNB[3][1]);
  series_UppLim->append(this->chartWidth, Limits_KPI_gNB[3][1]);
  series_UppLim->setColor(QColor(255, 0, 0));

  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  chart->legend()->hide();

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, 1.2*this->DLThrou.max_value);
  this->axisY->setTitleText("UL Throughput Mbit/sec");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if (this->DLThrou.plot_idx == 0){
    this->chart->addSeries(this->DLThrou.series);
    this->chartView->setChart(this->chart);
  }
  this->DLThrou.series->attachAxis(this->axisX);
  this->DLThrou.series->attachAxis(this->axisY);

  this->chart->addSeries(series_LowLim);
  series_LowLim->attachAxis(this->axisX);
  series_LowLim->attachAxis(this->axisY);

  this->chart->addSeries(series_UppLim);
  series_UppLim->attachAxis(this->axisX);
  series_UppLim->attachAxis(this->axisY);

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->DLThrou.plot_idx++;
  this->previousIndex = 8;
  makeConnections();
}

void PainterWidgetgNB::KPI_UL_Throu()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &targetUE->UE_sched_ctrl;
  uint32_t blockSize = (uint32_t)targetUE->mac_stats.ul.current_bytes;
  float bler_ul = (float)sched_ctrl->ul_bler_stats.bler;

  NR_DL_FRAME_PARMS *frame_parms = &this->p->gNB->frame_parms;
  uint16_t slots_per_frame = frame_parms->slots_per_frame;
  float slotDuration = 10.0/(float)slots_per_frame;      // slot duration in msec

  double blerTerm = 1.0 - (double)bler_ul;
  double blockSizeBits = (double)(blockSize << 3);

  double ThrouputKBitSec = blerTerm * blockSizeBits / (double)slotDuration;

  if ((this->ULThrou.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->ULThrou.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->ULThrou);
  }

  float Xpaint, Ypaint;
  Xpaint = this->ULThrou.plot_idx;
  Ypaint = (float)(ThrouputKBitSec/1000);    // Throughput in MBit/sec

  this->ULThrou.max_value = std::max(this->ULThrou.max_value, Ypaint);
  this->ULThrou.series->append(Xpaint, Ypaint);

  QLineSeries *series_LowLim = new QLineSeries();
  series_LowLim->append(0, Limits_KPI_gNB[1][0]);
  series_LowLim->append(this->chartWidth, Limits_KPI_gNB[1][0]);
  series_LowLim->setColor(QColor(0, 255, 0));

  QLineSeries *series_UppLim = new QLineSeries();
  series_UppLim->append(0, Limits_KPI_gNB[1][1]);
  series_UppLim->append(this->chartWidth, Limits_KPI_gNB[1][1]);
  series_UppLim->setColor(QColor(255, 0, 0));

  this->chart->legend()->hide();
  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, 1.2*this->ULThrou.max_value);
  this->axisY->setTitleText("UL Throughput Mbit/sec");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if (this->ULThrou.plot_idx == 0){
    this->chart->addSeries(this->ULThrou.series);
    this->chartView->setChart(this->chart);
  }
  this->ULThrou.series->attachAxis(this->axisX);
  this->ULThrou.series->attachAxis(this->axisY);

  this->chart->addSeries(series_LowLim);
  series_LowLim->attachAxis(this->axisX);
  series_LowLim->attachAxis(this->axisY);

  this->chart->addSeries(series_UppLim);
  series_UppLim->attachAxis(this->axisX);
  series_UppLim->attachAxis(this->axisY);

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->ULThrou.plot_idx++;
  this->previousIndex = 7;
  makeConnections();
}

void PainterWidgetgNB::KPI_DL_MCS()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &targetUE->UE_sched_ctrl;

  if ((this->DLMCS.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->DLMCS.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->DLMCS);

    if(this->indexToPlot != this->previousIndex)
      resetKPIValues(&this->DLMCS);
  }

  float Xpaint, Ypaint;
  Xpaint = this->DLMCS.plot_idx;
  Ypaint = (float)sched_ctrl->dl_bler_stats.mcs;

  this->DLMCS.max_value = std::max(this->DLMCS.max_value, Ypaint);
  this->DLMCS.min_value = std::min(this->DLMCS.min_value, Ypaint);

  this->DLMCS.series->append(Xpaint, Ypaint);
  this->DLMCS.seriesMin->append(Xpaint, this->DLMCS.min_value);
  this->DLMCS.seriesMax->append(Xpaint, this->DLMCS.max_value);

  chart->legend()->show();
  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, this->DLMCS.max_value + 2.0);
  this->axisY->setTitleText("DL MCS");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if (this->DLMCS.plot_idx == 0)
    this->chart->addSeries(this->DLMCS.series);
  this->DLMCS.series->attachAxis(this->axisX);
  this->DLMCS.series->attachAxis(this->axisY);

  if (this->DLMCS.plot_idx == 0)
    this->chart->addSeries(this->DLMCS.seriesMin);
  this->DLMCS.seriesMin->attachAxis(this->axisX);
  this->DLMCS.seriesMin->attachAxis(this->axisY);

  if (this->DLMCS.plot_idx == 0)
    this->chart->addSeries(this->DLMCS.seriesMax);
  this->DLMCS.seriesMax->attachAxis(this->axisX);
  this->DLMCS.seriesMax->attachAxis(this->axisY);

  if (this->DLMCS.plot_idx == 0)
    this->chartView->setChart(this->chart);
  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->DLMCS.plot_idx++;
  this->previousIndex = 5;
  makeConnections();
}

void PainterWidgetgNB::KPI_DL_BLER()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &targetUE->UE_sched_ctrl;

  if ((this->DLBLER.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->DLBLER.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->DLBLER);
  }

  float Xpaint, Ypaint;
  Xpaint = this->DLBLER.plot_idx;
  Ypaint = (float)sched_ctrl->dl_bler_stats.bler;
  this->DLBLER.series->append(Xpaint, Ypaint);

  QLineSeries *series_LowLim = new QLineSeries();
  series_LowLim->append(0, Limits_KPI_gNB[2][0]);
  series_LowLim->append(this->chartWidth, Limits_KPI_gNB[2][0]);
  series_LowLim->setColor(QColor(0, 255, 0));

  QLineSeries *series_UppLim = new QLineSeries();
  series_UppLim->append(0, Limits_KPI_gNB[2][1]);
  series_UppLim->append(this->chartWidth, Limits_KPI_gNB[2][1]);
  series_UppLim->setColor(QColor(255, 0, 0));

  chart->legend()->hide();
  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index (calc window: 100 ms)");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, 1.5);
  this->axisY->setTitleText("DL BLER");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if(this->DLBLER.plot_idx == 0){
    this->chart->addSeries(this->DLBLER.series);
    this->chartView->setChart(this->chart);
  }
  this->DLBLER.series->attachAxis(this->axisX);
  this->DLBLER.series->attachAxis(this->axisY);

  this->chart->addSeries(series_LowLim);
  series_LowLim->attachAxis(this->axisX);
  series_LowLim->attachAxis(this->axisY);

  this->chart->addSeries(series_UppLim);
  series_UppLim->attachAxis(this->axisX);
  series_UppLim->attachAxis(this->axisY);

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->DLBLER.plot_idx++;
  this->isOpenGLUsed = false;
  this->previousIndex = 4;

  makeConnections();
}

void PainterWidgetgNB::KPI_UL_BLER()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &targetUE->UE_sched_ctrl;

  if ((this->ULBLER.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->ULBLER.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->ULBLER);
  }

  float Xpaint, Ypaint;
  Xpaint = this->ULBLER.plot_idx;
  Ypaint = (float)sched_ctrl->ul_bler_stats.bler;
  this->ULBLER.series->append(Xpaint, Ypaint);

  QLineSeries *series_LowLim = new QLineSeries();
  series_LowLim->append(0, Limits_KPI_gNB[0][0]);
  series_LowLim->append(this->chartWidth, Limits_KPI_gNB[0][0]);
  series_LowLim->setColor(QColor(0, 255, 0));

  QLineSeries *series_UppLim = new QLineSeries();
  series_UppLim->append(0, Limits_KPI_gNB[0][1]);
  series_UppLim->append(this->chartWidth, Limits_KPI_gNB[0][1]);
  series_UppLim->setColor(QColor(255, 0, 0));

  this->chart->legend()->hide();
  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index (calc window: 100 ms)");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, 1.5);
  this->axisY->setTitleText("UL BLER");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if (this->ULBLER.plot_idx == 0){
    this->chart->addSeries(this->ULBLER.series);
    this->chartView->setChart(this->chart);
  }
  this->ULBLER.series->attachAxis(this->axisX);
  this->ULBLER.series->attachAxis(this->axisY);

  this->chart->addSeries(series_LowLim);
  series_LowLim->attachAxis(this->axisX);
  series_LowLim->attachAxis(this->axisY);

  this->chart->addSeries(series_UppLim);
  series_UppLim->attachAxis(this->axisX);
  series_UppLim->attachAxis(this->axisY);

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->previousIndex = 3;
  this->ULBLER.plot_idx++;
  makeConnections();
}

void PainterWidgetgNB::KPI_UL_MCS()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();

  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &targetUE->UE_sched_ctrl;

  if ((this->ULMCS.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->ULMCS.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->ULMCS);

    if (this->indexToPlot != this->previousIndex)
      resetKPIValues(&this->ULMCS);
  }

  float Xpaint, Ypaint;
  Xpaint = this->ULMCS.plot_idx;
  Ypaint = (float)sched_ctrl->ul_bler_stats.mcs;

  this->ULMCS.max_value = std::max(this->ULMCS.max_value, Ypaint);
  this->ULMCS.min_value = std::min(this->ULMCS.min_value, Ypaint);

  this->ULMCS.series->append(Xpaint, Ypaint);
  this->ULMCS.seriesMin->append(Xpaint, this->ULMCS.min_value);
  this->ULMCS.seriesMax->append(Xpaint, this->ULMCS.max_value);

  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->show();

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, this->ULMCS.max_value + 2.0);
  this->axisY->setTitleText("UL MCS");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if(this->ULMCS.plot_idx == 0)
    this->chart->addSeries(this->ULMCS.series);
  this->ULMCS.series->attachAxis(this->axisX);
  this->ULMCS.series->attachAxis(this->axisY);

  if(this->ULMCS.plot_idx == 0)
    this->chart->addSeries(this->ULMCS.seriesMin);
  this->ULMCS.seriesMin->attachAxis(this->axisX);
  this->ULMCS.seriesMin->attachAxis(this->axisY);

  if(this->ULMCS.plot_idx == 0)
    this->chart->addSeries(this->ULMCS.seriesMax);
  this->ULMCS.seriesMax->attachAxis(this->axisX);
  this->ULMCS.seriesMax->attachAxis(this->axisY);

  if(this->ULMCS.plot_idx == 0)
    this->chartView->setChart(this->chart);
  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->ULMCS.plot_idx++;
  this->previousIndex = 6;
  makeConnections();
}

void PainterWidgetgNB::KPI_PuschIQ()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->previousIndex = 0;

  //paint the axis and I/Q samples
  NR_DL_FRAME_PARMS *frame_parms=&this->p->gNB->frame_parms;
  int sz=frame_parms->N_RB_UL*12*frame_parms->symbols_per_slot;

  int ue = 0;
  float *I, *Q;
  float FIinit[sz] = { 0 }, FQinit[sz] = { 0 };
  I = FIinit;
  Q = FQinit;

  if ((this->p->gNB->pusch_vars) &&
      (this->p->gNB->pusch_vars[ue]) &&
      (this->p->gNB->pusch_vars[ue]->rxdataF_comp) &&
      (this->p->gNB->pusch_vars[ue]->rxdataF_comp[0]))
  {
    scopeSample_t *pusch_comp = (scopeSample_t *) this->p->gNB->pusch_vars[ue]->rxdataF_comp[0];
    for (int k=0; k<sz; k++ )
    {
      I[k] = pusch_comp[k].r;
      Q[k] = pusch_comp[k].i;
    }
  }

  QColor MarkerColor(0, 255, 0);
  const QString xLabel = QString("Real");
  const QString yLabel = QString("Img");
  createPixMap(I, Q, sz, MarkerColor, xLabel, yLabel, true);
}

void PainterWidgetgNB::KPI_PuschLLR()
{
    // erase the previous paint
    this->pix->fill(QColor(240,240,240));
    this->previousIndex = 1;

    //paint the axis LLRs
    int coded_bits_per_codeword =3*8*6144+12;
    int sz = coded_bits_per_codeword;
    int ue = 0;

    float *llr, *bit;
    float llrBuffer[sz] = { 0 }, bitBuffer[sz] = { 0 };
    llr = llrBuffer;
    bit = bitBuffer;

    if ((this->p->gNB->pusch_vars) &&
        (this->p->gNB->pusch_vars[ue]) &&
        (this->p->gNB->pusch_vars[ue]->llr))
    {
      int16_t *pusch_llr = (int16_t *)p->gNB->pusch_vars[ue]->llr;
      for (int i=0; i<sz; i++)
      {
        llr[i] = (float) pusch_llr[i];
        bit[i] = (float) i;
      }
      this->current_instance++;
    }

    QColor MarkerColor(0, 255, 0);
    const QString xLabel = QString("Sample Index");
    const QString yLabel = QString("LLR");
    createPixMap(bit, llr, sz, MarkerColor, xLabel, yLabel, false);
}

void PainterWidgetgNB::KPI_ChannelResponse()
{
  this->pix->fill(QColor(240,240,240));
  this->previousIndex = 2;

  const int len=2*this->p->gNB->frame_parms.ofdm_symbol_size;
  float *values, *time;
  float valuesBuffer[len] = { 0 }, timeBuffer[len] = { 0 };
  values = valuesBuffer;
  time = timeBuffer;
  const int ant=0;

  int ue = 0;

  if ((this->p->gNB->pusch_vars && p->gNB->pusch_vars[ue]) &&
      (this->p->gNB->pusch_vars[ue]->ul_ch_estimates_time) &&
      (this->p->gNB->pusch_vars[ue]->ul_ch_estimates_time[ant]))
  {
    scopeSample_t *data= (scopeSample_t *)this->p->gNB->pusch_vars[ue]->ul_ch_estimates_time[ant];

    if (data != NULL)
    {
      for (int i=0; i<len; i++)
      {
        values[i] = SquaredNorm(data[i]);
        time[i] = (float) i;
      }
      this->current_instance++;
    }
  }
  float maxY=0, minY=0;
  for (int k=0; k<len; k++) {
    maxY=std::max(maxY,values[k]);
    minY=std::min(minY,values[k]);
  }

  float maxYAbs = std::max(abs(maxY),abs(minY));
  QLineSeries *series = new QLineSeries();
  QColor MarkerColor(255, 0, 0);
  series->setColor(MarkerColor);
  series->setUseOpenGL(true);

  float Xpaint, Ypaint;

  float minYScaled=0, maxYScaled=0;
  for (int k=0; k<len; k++) {
    Xpaint = time[k];
    Ypaint = values[k]/maxYAbs*50;

    series->append(Xpaint, Ypaint);
    maxYScaled=std::max(maxYScaled,Ypaint);
    minYScaled=std::min(minYScaled,Ypaint);
  }

  this->chart->legend()->hide();
  this->chart->removeAllSeries();
  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , len);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange((minYScaled + 0.4*minYScaled), (maxYScaled + 0.4*maxYScaled));
  this->axisY->setTitleText("abs Channel");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  this->chart->addSeries(series);

  series->attachAxis(this->axisX);
  series->attachAxis(this->axisY);

  this->chartView->setChart(this->chart);
  this->chartView->resize(this->chartWidth, this->chartHight);
  this->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  makeConnections();
}

void PainterWidgetgNB::createPixMap(float *xData, float *yData, int len, QColor MarkerColor,
                                    const QString xLabel, const QString yLabel, bool scaleX)
{
  float maxX=0, maxY=0, minX=0, minY=0;
  for (int k=0; k<len; k++) {
    maxX=std::max(maxX,xData[k]);
    minX=std::min(minX,xData[k]);
    maxY=std::max(maxY,yData[k]);
    minY=std::min(minY,yData[k]);
  }

  float maxYAbs = std::max(abs(maxY),abs(minY));
  float maxXAbs = std::max(abs(maxX),abs(minX));

  QScatterSeries *series = new QScatterSeries();
  series->setUseOpenGL(true);
  series->setColor(MarkerColor);
  series->setMarkerSize(2);
  series->setBorderColor(Qt::transparent);
  series->setMarkerShape(QScatterSeries::MarkerShapeCircle);

  float Xpaint, Ypaint;

  QVector<QPointF> points(len);

  float minYScaled=0, maxYScaled=0, maxXScaled = 0, minXScaled = 0;
  for (int k=0; k<len; k++)
  {
    Ypaint = yData[k];
    Xpaint = xData[k];

    if (maxYAbs != 0)
      Ypaint = yData[k]/maxYAbs*50;

    if ((maxXAbs != 0) && (scaleX))
      Xpaint = xData[k]/maxXAbs*50;

    points[k] = QPointF(Xpaint, Ypaint);
    maxYScaled=std::max(maxYScaled,Ypaint);
    minYScaled=std::min(minYScaled,Ypaint);
    maxXScaled=std::max(maxXScaled,Xpaint);
    minXScaled=std::min(minXScaled,Xpaint);
  }

  series->replace(points);
  this->chart->removeAllSeries();
  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->hide();

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setTitleText(xLabel);
  this->axisY->setTickCount(nofTicks);
  this->axisY->setTitleText(yLabel);

  if (!scaleX)
  {
    this->axisX->setRange(0 , len);
    if ((minYScaled != 0) && (maxYScaled != 0))
    {
      if ((minYScaled < (this->previousScalingMin + 0.4*this->previousScalingMin)) ||
          (maxYScaled > (this->previousScalingMax + 0.4*this->previousScalingMax)))
      {
        this->axisY->setRange((minYScaled + 0.4*minYScaled), (maxYScaled + 0.4*maxYScaled));
        this->previousScalingMax = maxYScaled;
        this->previousScalingMin = minYScaled;
        std::cout << minYScaled << ", " << this->previousScalingMin << ", " << maxYScaled << ", " << this->previousScalingMax << std::endl;
      }
      else
      {
        this->axisY->setRange((this->previousScalingMin + 0.4*this->previousScalingMin),
        (this->previousScalingMax + 0.4*this->previousScalingMax));
      }
    }
    else
      this->axisY->setRange(-10, 10);
  }
  else
  {
    float maxAbs = std::max(maxXScaled, maxYScaled);
    float minAbs = std::min(minYScaled, minXScaled);
    if ((maxAbs != 0) && (minAbs != 0))
    {
      if ((minAbs < (this->previousScalingMin + 0.4*this->previousScalingMin)) ||
          (maxAbs > (this->previousScalingMax + 0.4*this->previousScalingMax)))
      {
        this->axisY->setRange((minAbs + 0.4*minAbs), (maxAbs + 0.4*maxAbs));
        this->axisX->setRange((minAbs + 0.4*minAbs) , (maxAbs + 0.4*maxAbs));
        this->previousScalingMax = maxYScaled;
        this->previousScalingMin = minYScaled;
      }
      else
      {
        this->axisX->setRange((this->previousScalingMin + 0.4*this->previousScalingMin),
        (this->previousScalingMax + 0.4*this->previousScalingMax));
        this->axisY->setRange((this->previousScalingMin + 0.4*this->previousScalingMin),
        (this->previousScalingMax + 0.4*this->previousScalingMax));
      }
    }
    else
    {
      this->axisY->setRange(-10, 10);
      this->axisX->setRange(-10,10);
    }

  }

  this->chart->addAxis(this->axisX, Qt::AlignBottom);
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  this->chart->addSeries(series);
  series->attachAxis(this->axisX);
  series->attachAxis(this->axisY);
  this->chartView->setChart(this->chart);
  this->isOpenGLUsed = true;

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  makeConnections();
}

void PainterWidget::paintPixmap_ueWaterFallTime()
{
  this->chartView->hide();
  this->isWaterFallTimeActive = true;
  QPainter PixPainter(this->pix);
  PixPainter.translate(0, this->pix->height()/4);

  scopeSample_t *values = (scopeSample_t *) this->ue->common_vars.rxdata[0];
  const int datasize = this->ue->frame_parms.samples_per_frame;

  if (values == NULL)
    return;

  this->waterFallh = this->chartHight/2 - 15;
  const int samplesPerPixel = datasize/this->chartWidth;
  int displayPart = this->waterFallh - ScaleZone;

  int row = this->iteration%displayPart;
  double avg=0;

  for (int i=0; i < displayPart; i++)
    avg+=this->waterFallAvg[i];

  avg/=displayPart;
  this->waterFallAvg[row]=0;

  for (int pix=0; pix<this->chartWidth; pix++)
  {
    scopeSample_t *end=values+(pix+1)*samplesPerPixel;
    end-=2;

    double val=0;

    for (scopeSample_t *s=values+(pix)*samplesPerPixel; s <end; s++)
      val += SquaredNorm(*s);

    val/=samplesPerPixel;
    this->waterFallAvg[row]+=val/this->chartWidth;

    if (val > avg*2 )
    {
      QColor IQColor(0,0,255);
      PixPainter.setPen(IQColor);
    }

    if (val > avg*10 ){
      QColor IQColor(0,255,0);
      PixPainter.setPen(IQColor);
    }

    if (val > avg*100 ){
      QColor IQColor(255,255,0);
      PixPainter.setPen(IQColor);
    }

    PixPainter.drawEllipse( QPoint(pix, this->iteration%displayPart), 2, 2 );
  }

  // Plot vertical Lines
  NR_DL_FRAME_PARMS *frame_parms = &this->ue->frame_parms;
  const float verticalSpacing = (float)this->chartWidth / (float)frame_parms->slots_per_frame;

  float startPointUp = -5;
  float startPointDown = startPointUp + this->waterFallh;
  for (uint16_t i = 0; i < frame_parms->slots_per_frame; i++)
  {
    float lineX = (float)i * verticalSpacing;
    QColor IQColor(0,0,0);
    PixPainter.setPen(IQColor);
    PixPainter.drawLine(QPoint(lineX, startPointUp), QPoint(lineX, startPointUp + 5));
    PixPainter.drawLine(QPoint(lineX, startPointDown), QPoint(lineX, startPointDown + 5));
  }

  this->iteration++;
  this->previousIndex = 6;
  this->isOpenGLUsed = false;
  repaint();
}

void PainterWidget::resetKPIPlot(KPI_elements *inputStruct)
{
  inputStruct->series = new QLineSeries();
  inputStruct->series->setColor(QColor(0,0,0));

  inputStruct->seriesMin = new QLineSeries();
  inputStruct->seriesMin->setColor(QColor(255,0,0));
  inputStruct->seriesMin->setName("Min.");

  inputStruct->seriesMax = new QLineSeries();
  inputStruct->seriesMax->setColor(QColor(0,255,0));
  inputStruct->seriesMax->setName("Max.");

  inputStruct->seriesAvg = new QLineSeries();
  inputStruct->seriesAvg->setColor(QColor(0,0,255));
  inputStruct->seriesAvg->setName("Avg.");
}

void PainterWidget::resetKPIValues(KPI_elements *inputStruct)
{
  inputStruct->max_value = 0.0;
  inputStruct->min_value = 0.0;
  inputStruct->avg_value = 0.0;
  inputStruct->avg_idx = 0;
  inputStruct->nof_retrans = 0;
}

PainterWidget::PainterWidget(QComboBox *parent, PHY_VARS_NR_UE *ue)
{
    timer = new QTimer(this);
    timerWaterFallTime = new QTimer(this);
    this->chartHight = 300;
    this->chartWidth = 300;
    this->pix = new QPixmap(this->chartWidth,this->chartHight);
    this->pix->fill(QColor(240,240,240));
    this->ue = ue;
    this->previousScalingMin = 200000;   // large enough to be overwritten in the first check
    this->previousScalingMax = -200000;  // small enough to be overwritten in the first check

    this->parentWindow = parent;

    QScatterSeries *series = new QScatterSeries();
    this->chart = new QChart();
    this->chart->addSeries(series);
    this->chartView = new QChartView(this->chart, this);
    this->chartView->resize(this->chartWidth, this->chartHight);
    this->resize(this->chartWidth, this->chartHight);
    this->isOpenGLUsed = false;
    this->chartView->hide();
    this->axisX = new QValueAxis;
    this->axisY = new QValueAxis;
    this->chart->addAxis(this->axisX, Qt::AlignBottom);
    this->chart->addAxis(this->axisY, Qt::AlignLeft);

    // settings for waterfall graph
    this->iteration = 0;
    this->waterFallh = this->chartHight/2 - 15;
    this->waterFallAvg= (double*) malloc(sizeof(*this->waterFallAvg) * this->waterFallh);
    this->isWaterFallTimeActive = false;

    for (int i=0; i< this->waterFallh; i++)
      this->waterFallAvg[i]=0;

    this->previousIndex = -1;
    this->indexToPlot = this->parentWindow->currentIndex();

    this->extendKPIUE.DL_BLER = -1;
    this->extendKPIUE.blockSize = 0;
    this->extendKPIUE.dl_mcs = 100;
    this->extendKPIUE.nofRBs = 34;

    // DL BLER
    this->DLBLER.plot_idx = 0;
    resetKPIPlot(&this->DLBLER);

    // DL MCS
    this->DLMCS.plot_idx = 0;
    resetKPIPlot(&this->DLMCS);
    resetKPIValues(&this->DLMCS);

    // Throughput
    this->Throu.plot_idx = 0;
    resetKPIPlot(&this->Throu);
    resetKPIValues(&this->Throu);

    // nof scheduled RBs
    this->nofRBs.plot_idx = 0;
    resetKPIPlot(&this->nofRBs);
    resetKPIValues(&this->nofRBs);

    makeConnections();
}

void PainterWidget::makeConnections()
{
  this->indexToPlot = this->parentWindow->currentIndex();
  getKPIUE(&this->extendKPIUE);
  disconnect(timer, nullptr, nullptr, nullptr);
  disconnect(timerWaterFallTime, nullptr, nullptr, nullptr);

  if ((this->indexToPlot != this->previousIndex) && (this->indexToPlot == 6))   // reset settings
  {
    this->pix->fill(QColor(240,240,240));
    this->iteration = 0;
    for (int i=0; i< this->waterFallh; i++)
      this->waterFallAvg[i]=0;
  }

  if (this->indexToPlot == 0)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::paintPixmap_uePbchIQ);
  }
  else if (this->indexToPlot == 1)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::paintPixmap_uePbchLLR);
  }
  else if (this->indexToPlot == 2)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::paintPixmap_uePdschIQ);
  }
  else if (this->indexToPlot == 3)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::paintPixmap_uePdschLLR);
  }
  else if (this->indexToPlot == 4)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::paintPixmap_uePdcchIQ);
  }
  else if (this->indexToPlot == 5)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::paintPixmap_uePdcchLLR);
  }
  else if (this->indexToPlot == 6)
  {
    connect(timerWaterFallTime, &QTimer::timeout, this, &PainterWidget::paintPixmap_ueWaterFallTime);    // water fall time domain
  }
  else if (this->indexToPlot == 7)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::paintPixmap_ueChannelResponse);    // Channel Response
  }
  else if (this->indexToPlot == 8)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::KPI_DL_BLER);
  }
  else if (this->indexToPlot == 9)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::KPI_DL_Throu);
  }
  else if (this->indexToPlot == 10)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::KPI_DL_MCS);
  }
  else if (this->indexToPlot == 11)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::KPI_Nof_RBs);
  }
  else if (this->indexToPlot == 12)
  {
    connect(timer, &QTimer::timeout, this, &PainterWidget::KPI_FreqOff_TimeAdv);
  }
  else if (this->indexToPlot == 13)
  {
    KPI_configurations();
  }
  timer->start(100);
  timerWaterFallTime->start(100);
}

void PainterWidget::KPI_configurations()
{
  QWidget *window_1 = new QWidget();
  window_1->resize(300, 300);
  window_1->setWindowTitle("UE Configs");
  configBoxgUE * configItem1 = new configBoxgUE(window_1, 0);
  configBoxgUE * configItem2 = new configBoxgUE(window_1, 1);
  configBoxgUE * configItem3 = new configBoxgUE(window_1, 2);
  configBoxgUE * configItem4 = new configBoxgUE(window_1, 3);
  QFormLayout *flo = new QFormLayout();
  flo->addRow("BLER lower",configItem1);
  flo->addRow("BLER upper",configItem2);
  flo->addRow("Throughput lower[Mbs]",configItem3);
  flo->addRow("Throughput upper[Mbs]",configItem4);
  window_1->setLayout(flo);
  window_1->show();
  //this->current_instance++;
}

void PainterWidget::createScatterPlot(float *xData, float *yData, int len,
                                      QColor MarkerColor, const QString xLabel,
                                      const QString yLabel, bool scaleX)
{
  float maxX=0, maxY=0, minX=0, minY=0;
  for (int k=0; k<len; k++) {
    maxX=std::max(maxX,xData[k]);
    minX=std::min(minX,xData[k]);
    maxY=std::max(maxY,yData[k]);
    minY=std::min(minY,yData[k]);
  }

  float maxYAbs = std::max(abs(maxY),abs(minY));
  float maxXAbs = std::max(abs(maxX),abs(minX));

  QScatterSeries *series = new QScatterSeries();
  series->setUseOpenGL(true);
  series->setColor(MarkerColor);
  series->setMarkerSize(2);
  series->setBorderColor(Qt::transparent);
  series->setMarkerShape(QScatterSeries::MarkerShapeCircle);

  float Xpaint, Ypaint;

  QVector<QPointF> points(len);

  float minYScaled=0, maxYScaled=0, maxXScaled = 0, minXScaled = 0;
  for (int k=0; k<len; k++)
  {
    Ypaint = yData[k];
    Xpaint = xData[k];

    if (maxYAbs != 0)
      Ypaint = yData[k]/maxYAbs*50;

    if ((maxXAbs != 0) && (scaleX))
      Xpaint = xData[k]/maxXAbs*50;

    points[k] = QPointF(Xpaint, Ypaint);
    maxYScaled=std::max(maxYScaled,Ypaint);
    minYScaled=std::min(minYScaled,Ypaint);
    maxXScaled=std::max(maxXScaled,Xpaint);
    minXScaled=std::min(minXScaled,Xpaint);
  }

  series->replace(points);
  this->chart->removeAllSeries();
  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->hide();

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setTitleText(xLabel);
  this->axisY->setTickCount(nofTicks);
  this->axisY->setTitleText(yLabel);

  if (!scaleX)
  {
    this->axisX->setRange(0 , len);
    if ((minYScaled != 0) && (maxYScaled != 0))
    {
      if ((minYScaled < (this->previousScalingMin + 0.4*this->previousScalingMin)) ||
          (maxYScaled > (this->previousScalingMax + 0.4*this->previousScalingMax)))
      {
        this->axisY->setRange((minYScaled + 0.4*minYScaled), (maxYScaled + 0.4*maxYScaled));
        this->previousScalingMax = maxYScaled;
        this->previousScalingMin = minYScaled;
        std::cout << minYScaled << ", " << this->previousScalingMin << ", " << maxYScaled << ", " << this->previousScalingMax << std::endl;
      }
      else
      {
        this->axisY->setRange((this->previousScalingMin + 0.4*this->previousScalingMin),
        (this->previousScalingMax + 0.4*this->previousScalingMax));
      }
    }
    else
      this->axisY->setRange(-10, 10);
  }
  else
  {
    float maxAbs = std::max(maxXScaled, maxYScaled);
    float minAbs = std::min(minYScaled, minXScaled);
    if ((maxAbs != 0) && (minAbs != 0))
    {
      if ((minAbs < (this->previousScalingMin + 0.4*this->previousScalingMin)) ||
          (maxAbs > (this->previousScalingMax + 0.4*this->previousScalingMax)))
      {
        this->axisY->setRange((minAbs + 0.4*minAbs), (maxAbs + 0.4*maxAbs));
        this->axisX->setRange((minAbs + 0.4*minAbs) , (maxAbs + 0.4*maxAbs));
        this->previousScalingMax = maxYScaled;
        this->previousScalingMin = minYScaled;
      }
      else
      {
        this->axisX->setRange((this->previousScalingMin + 0.4*this->previousScalingMin),
        (this->previousScalingMax + 0.4*this->previousScalingMax));
        this->axisY->setRange((this->previousScalingMin + 0.4*this->previousScalingMin),
        (this->previousScalingMax + 0.4*this->previousScalingMax));
      }
    }
    else
    {
      this->axisY->setRange(-10, 10);
      this->axisX->setRange(-10,10);
    }

  }

  this->chart->addAxis(this->axisX, Qt::AlignBottom);
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  this->chart->addSeries(series);
  series->attachAxis(this->axisX);
  series->attachAxis(this->axisY);
  this->chartView->setChart(this->chart);
  this->isOpenGLUsed = true;

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  makeConnections();
}

void PainterWidget::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.drawPixmap( (this->width()-this->pix->width())/2,
  (this->height()-this->pix->height())/2, *this->pix); // paint pixmap on widget

  makeConnections();
}

void PainterWidget::KPI_FreqOff_TimeAdv()
{
  this->chartView->hide();
  this->pix->fill(QColor(240,240,240));
  QPainter PixPainter(this->pix);
  PixPainter.translate(0, this->pix->height()/2);
  float maxYScaled = (float)this->chartHight;
  float freq_offset = (float)this->ue->common_vars.freq_offset;
  std::string strFreq = "Freq. Offset= " + std::to_string(freq_offset);
  QString qstrFreq = QString::fromStdString(strFreq);
  PixPainter.drawText(20, -0.2*maxYScaled, qstrFreq);

  float timing_advance = (float)this->ue->timing_advance;
  std::string strTime = "Timing Advance= " + std::to_string(timing_advance);
  QString qstrTime = QString::fromStdString(strTime);
  PixPainter.drawText(20, 0*maxYScaled, qstrTime);
  this->previousIndex = 12;
  this->isOpenGLUsed = false;
  update();
}

void PainterWidget::KPI_Nof_RBs()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();
  this->isOpenGLUsed = false;

  if ((this->nofRBs.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->nofRBs.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->nofRBs);
  }

  float Xpaint, Ypaint;
  Xpaint = this->nofRBs.plot_idx;
  Ypaint = (float)this->extendKPIUE.nofRBs;

  this->nofRBs.max_value = std::max(this->nofRBs.max_value, Ypaint);
  this->nofRBs.series->append(Xpaint, Ypaint);

  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->hide();

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, this->nofRBs.max_value + 10);
  this->axisY->setTitleText("Nof Scheduled RBs");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if (this->nofRBs.plot_idx == 0){
    this->chart->addSeries(this->nofRBs.series);
    this->chartView->setChart(this->chart);
  }
  this->nofRBs.series->attachAxis(this->axisX);
  this->nofRBs.series->attachAxis(this->axisY);

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->previousIndex = 11;
  this->nofRBs.plot_idx++;
  makeConnections();
}

void PainterWidget::KPI_DL_Throu()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();
  this->isOpenGLUsed = false;

  uint32_t blockSize = (uint32_t)this->extendKPIUE.blockSize;
  float bler_dl = this->extendKPIUE.DL_BLER;

  NR_DL_FRAME_PARMS *frame_parms = &this->ue->frame_parms;
  uint16_t slots_per_frame = frame_parms->slots_per_frame;
  float slotDuration = 10.0/(float)slots_per_frame;      // slot duration in msec

  double blerTerm = 1.0 - (double)bler_dl;
  double blockSizeBits = (double)(blockSize << 3);

  double ThrouputKBitSec = blerTerm * blockSizeBits / (double)slotDuration;

  if ((this->Throu.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->Throu.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->Throu);
  }

  float Xpaint, Ypaint;
  Xpaint = this->Throu.plot_idx;
  Ypaint = (float)(ThrouputKBitSec/1000);    // Throughput in MBit/sec

  this->Throu.max_value = std::max(this->Throu.max_value, Ypaint);
  this->Throu.series->append(Xpaint, Ypaint);

  QLineSeries *series_LowLim = new QLineSeries();
  series_LowLim->append(0, Limits_KPI_gNB[1][0]);
  series_LowLim->append(this->chartWidth, Limits_KPI_gNB[1][0]);
  series_LowLim->setColor(QColor(0, 255, 0));

  QLineSeries *series_UppLim = new QLineSeries();
  series_UppLim->append(0, Limits_KPI_gNB[1][1]);
  series_UppLim->append(this->chartWidth, Limits_KPI_gNB[1][1]);
  series_UppLim->setColor(QColor(255, 0, 0));

  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->hide();

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, 1.2*this->Throu.max_value);
  this->axisY->setTitleText("UL Throughput Mbit/sec");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if (this->Throu.plot_idx == 0){
    this->chart->addSeries(this->Throu.series);
    this->chartView->setChart(this->chart);
  }
  this->Throu.series->attachAxis(this->axisX);
  this->Throu.series->attachAxis(this->axisY);

  this->chart->addSeries(series_LowLim);
  series_LowLim->attachAxis(this->axisX);
  series_LowLim->attachAxis(this->axisY);

  this->chart->addSeries(series_UppLim);
  series_UppLim->attachAxis(this->axisX);
  series_UppLim->attachAxis(this->axisY);

  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->previousIndex = 9;
  this->Throu.plot_idx++;
  makeConnections();
}

void PainterWidget::KPI_DL_MCS()
{
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));
  this->chartView->hide();
  this->isOpenGLUsed = false;

  if ((this->DLMCS.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->DLMCS.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->DLMCS);

    if(this->indexToPlot != this->previousIndex)
      resetKPIValues(&this->DLMCS);
  }

  float Xpaint, Ypaint;
  Xpaint = this->DLMCS.plot_idx;
  Ypaint = (float)this->extendKPIUE.dl_mcs;

  this->DLMCS.max_value = std::max(this->DLMCS.max_value, Ypaint);
  this->DLMCS.min_value = std::min(this->DLMCS.min_value, Ypaint);

  this->DLMCS.series->append(Xpaint, Ypaint);
  this->DLMCS.seriesMin->append(Xpaint, this->DLMCS.min_value);
  this->DLMCS.seriesMax->append(Xpaint, this->DLMCS.max_value);

  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->show();

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, this->DLMCS.max_value + 2.0);
  this->axisY->setTitleText("DL MCS");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if (this->DLMCS.plot_idx == 0)
    this->chart->addSeries(this->DLMCS.series);
  
  this->DLMCS.series->attachAxis(this->axisX);
  this->DLMCS.series->attachAxis(this->axisY);
  
  if (this->DLMCS.plot_idx == 0)
    this->chart->addSeries(this->DLMCS.seriesMin);
  this->DLMCS.seriesMin->attachAxis(this->axisX);
  this->DLMCS.seriesMin->attachAxis(this->axisY);
  
  if (this->DLMCS.plot_idx == 0)
    this->chart->addSeries(this->DLMCS.seriesMax);
  this->DLMCS.seriesMax->attachAxis(this->axisX);
  this->DLMCS.seriesMax->attachAxis(this->axisY);

  if (this->DLMCS.plot_idx == 0)
    this->chartView->setChart(this->chart);
  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->previousIndex = 10;
  this->DLMCS.plot_idx++;
  makeConnections();
}

void PainterWidget::KPI_DL_BLER()
{
  // erase the previous paint
  this->chartView->hide();
  this->pix->fill(QColor(240,240,240));

  if ((this->DLBLER.plot_idx > this->chartWidth) ||
      (this->indexToPlot != this->previousIndex))
  {
    this->DLBLER.plot_idx = 0;
    this->chart->removeAllSeries();
    resetKPIPlot(&this->DLBLER);
  }

  float Xpaint, Ypaint;
  Xpaint = this->DLBLER.plot_idx;
  Ypaint = this->extendKPIUE.DL_BLER;
  this->DLBLER.series->append(Xpaint, Ypaint);

  QLineSeries *series_LowLim = new QLineSeries();
  series_LowLim->append(0, Limits_KPI_ue[0][0]);
  series_LowLim->append(this->chartWidth, Limits_KPI_ue[0][0]);
  series_LowLim->setColor(QColor(0, 255, 0));

  QLineSeries *series_UppLim = new QLineSeries();
  series_UppLim->append(0, Limits_KPI_ue[0][1]);
  series_UppLim->append(this->chartWidth, Limits_KPI_ue[0][1]);
  series_UppLim->setColor(QColor(255, 0, 0));
  
  this->chart->legend()->hide();
  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , this->chartWidth);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange(-1, 1.5);
  this->axisY->setTitleText("DL BLER");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  if(this->DLBLER.plot_idx == 0){
    this->chart->addSeries(this->DLBLER.series);
    this->chartView->setChart(this->chart);
  }
  this->DLBLER.series->attachAxis(axisX);
  this->DLBLER.series->attachAxis(axisY);

  this->chart->addSeries(series_LowLim);
  series_LowLim->attachAxis(this->axisX);
  series_LowLim->attachAxis(this->axisY);

  this->chart->addSeries(series_UppLim);
  series_UppLim->attachAxis(this->axisX);
  series_UppLim->attachAxis(this->axisY);
  
  this->chartView->resize(this->chartWidth, this->chartHight);
  this->chartView->show();

  this->previousIndex = 8;
  this->isOpenGLUsed = false;
  this->DLBLER.plot_idx++;
  makeConnections();
}

void PainterWidget::paintPixmap_ueChannelResponse()
{
  this->previousIndex = 7;
  scopeData_t *scope=(scopeData_t *) this->ue->scopeData;
  scope->flag_streaming[pbchDlChEstimateTime] = 1;

  scopeGraphData_t **UELiveData = (scopeGraphData_t**)(( scopeData_t *)this->ue->scopeData)->liveDataUE;

  if (!UELiveData[pbchDlChEstimateTime])
    return;

  const scopeSample_t *tmp=(scopeSample_t *)(UELiveData[pbchDlChEstimateTime]+1);
  const scopeSample_t **data=&tmp;
  const int len = UELiveData[pbchDlChEstimateTime]->lineSz;

  float *values, *time;
  float valuesBuffer[len] = { 0 }, timeBuffer[len] = { 0 };
  values = valuesBuffer;
  time = timeBuffer;

  int idx = 0;
  int nb_ant = 1;
  for (int ant=0; ant<nb_ant; ant++)
  {
    if (data[ant] != NULL)
    {
      for (int i=0; i<len; i++)
      {
        values[i] = SquaredNorm(data[ant][i]);
        time[i] = (float) idx;
        idx++;
      }
    }
  }

  float maxY=0, minY=0;
  for (int k=0; k<len; k++) {
    maxY=std::max(maxY,values[k]);
    minY=std::min(minY,values[k]);
  }

  float maxYAbs = std::max(abs(maxY),abs(minY));

  QLineSeries *series = new QLineSeries();
  series->setUseOpenGL(true);
  QColor MarkerColor(255, 0, 0);
  series->setColor(MarkerColor);

  float Xpaint, Ypaint;
  float minYScaled=0, maxYScaled=0;
  for (int k=0; k<len; k++) {
    Xpaint = time[k];
    Ypaint = values[k]/maxYAbs*50;

    series->append(Xpaint, Ypaint);

    maxYScaled=std::max(maxYScaled,Ypaint);
    minYScaled=std::min(minYScaled,Ypaint);
  }

  this->chart->removeAllSeries();
  this->chart->removeAxis(this->axisX);
  this->chart->removeAxis(this->axisY);
  this->chart->legend()->hide();

  int nofTicks = 6;
  this->axisX->setTickCount(nofTicks);
  this->axisX->setRange(0 , len);
  this->axisX->setTitleText("Time Index");
  this->chart->addAxis(this->axisX, Qt::AlignBottom);

  this->axisY->setTickCount(nofTicks);
  this->axisY->setRange((minYScaled + 0.4*minYScaled), (maxYScaled + 0.4*maxYScaled));
  this->axisY->setTitleText("abs Channel");
  this->chart->addAxis(this->axisY, Qt::AlignLeft);

  this->chart->addSeries(series);
  series->attachAxis(this->axisX);
  series->attachAxis(this->axisY);

  this->chartView->setChart(this->chart);

  this->isOpenGLUsed = true;
  this->chartView->resize(this->chartWidth, this->chartHight);
  this->resize(this->chartWidth, this->chartHight);
  this->chartView->show();
  makeConnections();
}

void PainterWidget::paintPixmap_uePdschIQ()
{
  this->previousIndex = 2;
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));

  //paint the axis and I/Q samples
  if (!this->ue->pdsch_vars[0]->rxdataF_comp0[0])
      return;

  NR_DL_FRAME_PARMS *frame_parms = &this->ue->frame_parms;
  int sz = frame_parms->symbols_per_slot*frame_parms->N_RB_DL*12; // size of the malloced buffer
  scopeSample_t *pdsch_comp = (scopeSample_t *) this->ue->pdsch_vars[0]->rxdataF_comp0[0];

  float *I, *Q;
  float FIinit[sz] = { 0 }, FQinit[sz] = { 0 };
  I = FIinit;
  Q = FQinit;

  for (int s=0; s<sz; s++) {
    I[s] += pdsch_comp[s].r;
    Q[s] += pdsch_comp[s].i;
  }

  QColor MarkerColor(0, 255, 0);
  const QString xLabel = QString("Real");
  const QString yLabel = QString("Img");
  createScatterPlot(I, Q, sz, MarkerColor, xLabel, yLabel, true);
}

void PainterWidget::resizeEvent(QResizeEvent *event)
{
  if ((width() != this->chartWidth) || (height() != this->chartHight))
  {
    this->chartHight = height();
    this->chartWidth = width();

    // reset for waterfall plot
    if (this->indexToPlot == 6)
    {
      QPixmap *newPix = new QPixmap(this->chartWidth,this->chartHight);
      this->pix = newPix;
      this->pix->fill(QColor(240,240,240));
      this->iteration = 0;
      this->waterFallh = this->chartHight/2 - 15;
      for (int i=0; i< this->waterFallh; i++)
        this->waterFallAvg[i]=0;
    }

    update();
  }
  QWidget::resizeEvent(event);
}

void PainterWidget::paintPixmap_uePdcchIQ()
{
    this->previousIndex = 4;
    this->pix->fill(QColor(240,240,240));

    scopeData_t *scope=(scopeData_t *) this->ue->scopeData;
    scope->flag_streaming[pdcchRxdataF_comp] = 1;

    scopeGraphData_t **UELiveData = (scopeGraphData_t**)(( scopeData_t *)this->ue->scopeData)->liveDataUE;

    if (!UELiveData[pdcchRxdataF_comp])
      return;

    const int len=UELiveData[pdcchRxdataF_comp]->lineSz;

    scopeSample_t *pdcch_comp = (scopeSample_t *) (UELiveData[pdcchRxdataF_comp]+1);

    float *I, *Q;
    float FIinit[len] = { 0 }, FQinit[len] = { 0 };
    I = FIinit;
    Q = FQinit;

    for (int i=0; i<len; i++) {
      I[i] = pdcch_comp[i].r;
      Q[i] = pdcch_comp[i].i;
    }

    QColor MarkerColor(0, 0, 255);
    const QString xLabel = QString("Real");
    const QString yLabel = QString("Img");
    createScatterPlot(I, Q, len, MarkerColor, xLabel, yLabel, true);
}

void PainterWidget::paintPixmap_uePbchIQ()
{
    this->previousIndex = 0;
    // erase the previous paint
    this->pix->fill(QColor(240,240,240));

    scopeData_t *scope=(scopeData_t *) this->ue->scopeData;
    scope->flag_streaming[pbchRxdataF_comp] = 1;

    scopeGraphData_t **data = (scopeGraphData_t**)(( scopeData_t *)this->ue->scopeData)->liveDataUE;

    if (!data[pbchRxdataF_comp])
      return;

    const int len=data[pbchRxdataF_comp]->lineSz;
    scopeSample_t *pbch_comp = (scopeSample_t *) (data[pbchRxdataF_comp]+1);

    float *I, *Q;
    float FIinit[len] = { 0 }, FQinit[len] = { 0 };
    I = FIinit;
    Q = FQinit;

    for (int i=0; i<len; i++) {
      I[i] = pbch_comp[i].r;
      Q[i] = pbch_comp[i].i;
    }

    QColor MarkerColor(255, 0, 0);
    const QString xLabel = QString("Real");
    const QString yLabel = QString("Img");
    createScatterPlot(I, Q, len, MarkerColor, xLabel, yLabel, true);
}

void PainterWidget::paintPixmap_uePdschLLR()
{
  this->previousIndex = 3;
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));

  //paint the axis and LLR values
  if (!this->ue->pdsch_vars[0]->llr[0])
      return;

  NR_DL_FRAME_PARMS *frame_parms = &this->ue->frame_parms;
  int num_re = frame_parms->N_RB_DL*12*frame_parms->symbols_per_slot;
  int Qm = 2;
  int coded_bits_per_codeword = num_re*Qm;
  float *llr, *bit;
  float FBitinit[coded_bits_per_codeword] = { 0 }, FLlrinit[coded_bits_per_codeword] = { 0 };
  bit = FBitinit;
  llr = FLlrinit;

  int16_t *pdsch_llr = (int16_t *) this->ue->pdsch_vars[0]->llr[0]; // stream 0
  for (int i=0; i<coded_bits_per_codeword; i++)
  {
    llr[i] = (float) pdsch_llr[i];
    bit[i] = (float) i;
  }

  QColor MarkerColor(0, 255, 0);
  const QString xLabel = QString("Sample Index");
  const QString yLabel = QString("LLR");
  createScatterPlot(bit, llr, coded_bits_per_codeword, MarkerColor, xLabel, yLabel, false);
}

void PainterWidget::paintPixmap_uePdcchLLR()
{
  this->previousIndex = 5;
  // erase the previous paint
  this->pix->fill(QColor(240,240,240));

  scopeData_t *scope=(scopeData_t *) this->ue->scopeData;
  scope->flag_streaming[pdcchLlr] = 1;

  scopeGraphData_t **data = (scopeGraphData_t**)(( scopeData_t *)this->ue->scopeData)->liveDataUE;

  if (!data[pdcchLlr])
      return;

  const int len=data[pdcchLlr]->lineSz;
  float *llr, *bit;
  float FBitinit[len] = { 0 }, FLlrinit[len] = { 0 };
  bit = FBitinit;
  llr = FLlrinit;

  int16_t *pdcch_llr = (int16_t *)(data[pdcchLlr]+1);

  for (int i=0; i<len; i++)
  {
    llr[i] = (float) pdcch_llr[i];
    bit[i] = (float) i;
  }

  QColor MarkerColor(0, 0, 255);
  const QString xLabel = QString("Sample Index");
  const QString yLabel = QString("LLR");
  createScatterPlot(bit, llr, len, MarkerColor, xLabel, yLabel, false);
}

void PainterWidget::paintPixmap_uePbchLLR()
{
  this->previousIndex = 1;
  this->pix->fill(QColor(240,240,240));

  scopeData_t *scope=(scopeData_t *) this->ue->scopeData;
  scope->flag_streaming[pbchLlr] = 1;

  scopeGraphData_t **data = (scopeGraphData_t**)(( scopeData_t *)this->ue->scopeData)->liveDataUE;

  if (!data[pbchLlr])
      return;

  const int len=data[pbchLlr]->lineSz;
  float *llr, *bit;
  float FBitinit[len] = { 0 }, FLlrinit[len] = { 0 };
  bit = FBitinit;
  llr = FLlrinit;

  int16_t *llr_pbch = (int16_t *)(data[pbchLlr]+1);
  for (int i=0; i<len; i++)
  {
    llr[i] = (float) llr_pbch[i];
    bit[i] = (float) i;
  }

  QColor MarkerColor(255, 0, 0);
  const QString xLabel = QString("Sample Index");
  const QString yLabel = QString("LLR");
  createScatterPlot(bit, llr, len, MarkerColor, xLabel, yLabel, false);
}

void *nrUEQtscopeThread(void *arg)
{
	PHY_VARS_NR_UE *ue=(PHY_VARS_NR_UE *)arg;

	int argc = 1;
	char *argv[] = { (char*)"nrqt_scopeUE"};
	QApplication a(argc, argv);

  // Create a main window (widget)
  QWidget *window = new QWidget();
  window->setBaseSize(QSize(600, 900));
  window->resize(600, 900);
  window->setSizeIncrement(20, 20);
  // Window title
  window->setWindowTitle("UE Scope");

  // create the popup lists
  KPIListSelect * combo1 = new KPIListSelect(window);
  combo1->setCurrentIndex(0);
  KPIListSelect * combo2 = new KPIListSelect(window);
  combo2->setCurrentIndex(1);
  KPIListSelect * combo3 = new KPIListSelect(window);
  combo3->setCurrentIndex(2);
  KPIListSelect * combo4 = new KPIListSelect(window);
  combo4->setCurrentIndex(3);
  KPIListSelect * combo5 = new KPIListSelect(window);
  combo5->setCurrentIndex(4);
  KPIListSelect * combo6 = new KPIListSelect(window);
  combo6->setCurrentIndex(7);

  // Initial Plots
  PainterWidget pwidgetueCombo3(combo3, ue);   // I/Q PDSCH
  PainterWidget pwidgetueCombo4(combo4, ue);   // LLR PDSCH */
  PainterWidget pwidgetueCombo1(combo1, ue);   // I/Q PBCH
  PainterWidget pwidgetueCombo2(combo2, ue);   // LLR PBCH
  PainterWidget pwidgetueCombo5(combo5, ue);   // I/Q PDCCH
  PainterWidget pwidgetueCombo6(combo6, ue);   // LLR PDCCH

  while (true)
  {
    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout;

    // left and right layouts
    QVBoxLayout *LeftLayout = new QVBoxLayout;
    QVBoxLayout *RightLayout = new QVBoxLayout;


    LeftLayout->addWidget(combo1);
    LeftLayout->addWidget(&pwidgetueCombo1,20);
    LeftLayout->addStretch(); // space for later plots

    LeftLayout->addWidget(combo3);
    LeftLayout->addWidget(&pwidgetueCombo3,20);
    LeftLayout->addStretch(); // space for later plots

    LeftLayout->addWidget(combo5);
    LeftLayout->addWidget(&pwidgetueCombo5,20);
    LeftLayout->addStretch(); // space for later plots

    RightLayout->addWidget(combo2);
    RightLayout->addWidget(&pwidgetueCombo2,20);
    RightLayout->addStretch(); // space for later plots

    RightLayout->addWidget(combo4);
    RightLayout->addWidget(&pwidgetueCombo4,20);
    RightLayout->addStretch(); // space for later plots

    RightLayout->addWidget(combo6);
    RightLayout->addWidget(&pwidgetueCombo6,20);
    RightLayout->addStretch(); // space for later plots

    // add the left and right layouts to the main layout
    mainLayout->addLayout(LeftLayout);
    mainLayout->addLayout(RightLayout);

    // set the layout of the main window
    window->setLayout(mainLayout);

    pwidgetueCombo1.makeConnections();
    pwidgetueCombo2.makeConnections();
    pwidgetueCombo3.makeConnections();
    pwidgetueCombo4.makeConnections();
    pwidgetueCombo5.makeConnections();
    pwidgetueCombo6.makeConnections();

    // display the main window
    window->show();
    a.exec();
  }

  pthread_exit((void *)arg);
	//return NULL;
}

void *nrgNBQtscopeThread(void *arg)
{
  scopeData_t *p=(scopeData_t *)arg;
	int argc = 1;
	char *argv[] = { (char*)"nrqt_scopegNB"};
	QApplication a(argc, argv);

  // Create a main window (widget)
  QWidget *window = new QWidget();
  window->resize(600, 900);
  // Window title
  window->setWindowTitle("gNB Scope");

  // create the popup lists
  KPIListSelectgNB * combo1 = new KPIListSelectgNB(window);
  combo1->setCurrentIndex(0);
  KPIListSelectgNB * combo2 = new KPIListSelectgNB(window);
  combo2->setCurrentIndex(1);
  KPIListSelectgNB * combo3 = new KPIListSelectgNB(window);
  combo3->setCurrentIndex(2);
  KPIListSelectgNB * combo4 = new KPIListSelectgNB(window);
  combo4->setCurrentIndex(3);
  KPIListSelectgNB * combo5 = new KPIListSelectgNB(window);
  combo5->setCurrentIndex(4);
  KPIListSelectgNB * combo6 = new KPIListSelectgNB(window);
  combo6->setCurrentIndex(5);

  PainterWidgetgNB pwidgetgnbCombo1(combo1, p);   // I/Q PUSCH
  PainterWidgetgNB pwidgetgnbCombo2(combo2, p);   // LLR PUSCH
  PainterWidgetgNB pwidgetgnbCombo3(combo3, p);   // Channel Response
  PainterWidgetgNB pwidgetgnbCombo4(combo4, p);   //
  PainterWidgetgNB pwidgetgnbCombo5(combo5, p);   //
  PainterWidgetgNB pwidgetgnbCombo6(combo6, p);   //

  while (true)
  {
    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout;

    // left and right layouts
    QVBoxLayout *LeftLayout = new QVBoxLayout;
    QVBoxLayout *RightLayout = new QVBoxLayout;

    LeftLayout->addWidget(combo1);
    LeftLayout->addWidget(&pwidgetgnbCombo1,20);
    LeftLayout->addStretch(); // space for later plots

    LeftLayout->addWidget(combo3);
    LeftLayout->addWidget(&pwidgetgnbCombo3,20);
    LeftLayout->addStretch(); // space for later plots

    LeftLayout->addWidget(combo5);
    LeftLayout->addWidget(&pwidgetgnbCombo5,20);
    LeftLayout->addStretch(); // space for later plots

    RightLayout->addWidget(combo2);
    RightLayout->addWidget(&pwidgetgnbCombo2,20);
    RightLayout->addStretch(); // space for later plots

    RightLayout->addWidget(combo4);
    RightLayout->addWidget(&pwidgetgnbCombo4,20);
    RightLayout->addStretch(); // space for later plots

    RightLayout->addWidget(combo6);      //
    RightLayout->addWidget(&pwidgetgnbCombo6,20);
    RightLayout->addStretch(); // space for later plots

    // add the left and right layouts to the main layout
    mainLayout->addLayout(LeftLayout);
    mainLayout->addLayout(RightLayout);

    // set the layout of the main window
    window->setLayout(mainLayout);

    pwidgetgnbCombo1.makeConnections();
    pwidgetgnbCombo2.makeConnections();
    pwidgetgnbCombo3.makeConnections();
    pwidgetgnbCombo4.makeConnections();
    pwidgetgnbCombo5.makeConnections();
    pwidgetgnbCombo6.makeConnections();

    // display the main window
    window->show();
    a.exec();
  }

	return NULL;
}

void UEcopyData(PHY_VARS_NR_UE *ue, enum UEdataType type, void *dataIn, int elementSz, int colSz, int lineSz) {

  // Local static copy of the scope data bufs
  // The active data buf is alterned to avoid interference between the Scope thread (display) and the Rx thread (data input)
  // Index of "2" could be set to the number of Rx threads + 1
  static scopeGraphData_t *copyDataBufs[UEdataTypeNumberOfItems][2] = {0};
  static int  copyDataBufsIdx[UEdataTypeNumberOfItems] = {0};

  scopeData_t *tmp=(scopeData_t *)ue->scopeData;

  if (tmp) {
    // Begin of critical zone between UE Rx threads that might copy new data at the same time: might require a mutex
    int newCopyDataIdx = (copyDataBufsIdx[type]==0)?1:0;
    copyDataBufsIdx[type] = newCopyDataIdx;
    // End of critical zone between UE Rx threads

    // New data will be copied in a different buffer than the live one
    scopeGraphData_t *copyData= copyDataBufs[type][newCopyDataIdx];

    if (copyData == NULL || copyData->dataSize < elementSz*colSz*lineSz) {
      scopeGraphData_t *ptr= (scopeGraphData_t*) realloc(copyData, sizeof(scopeGraphData_t) + elementSz*colSz*lineSz);

      if (!ptr) {
        LOG_E(PHY,"can't realloc\n");
        return;
      } else {
        copyData=ptr;
      }
    }

    copyData->dataSize=elementSz*colSz*lineSz;
    copyData->elementSz=elementSz;
    copyData->colSz=colSz;
    copyData->lineSz=lineSz;
    memcpy(copyData+1, dataIn,  elementSz*colSz*lineSz);
    copyDataBufs[type][newCopyDataIdx] = copyData;

    // The new data just copied in the local static buffer becomes live now
    ((scopeGraphData_t **)tmp->liveDataUE)[type]=copyData;
  }
}


void nrUEinitQtScope(PHY_VARS_NR_UE *ue) {
  ue->scopeData=malloc(sizeof(scopeData_t));
  scopeData_t *scope=(scopeData_t *) ue->scopeData;
  scope->copyData=UEcopyData;
  scope->liveDataUE=calloc(sizeof(scopeGraphData_t *), UEdataTypeNumberOfItems);
  for (int i=0; i<UEdataTypeNumberOfItems; i++){
    scope->flag_streaming[i] = 0;
  }
  pthread_t qtscope_thread;
  threadCreate(&qtscope_thread, nrUEQtscopeThread, ue, (char*)"qtscope", -1, sched_get_priority_min(SCHED_RR));
}

void nrgNBinitQtScope(scopeParms_t *p) {

  p->gNB->scopeData=malloc(sizeof(scopeData_t));
  scopeData_t *scope=(scopeData_t *) p->gNB->scopeData;
  scope->gNB=p->gNB;
  scope->argc=p->argc;
  scope->argv=p->argv;
  scope->ru=p->ru;

  pthread_t qtscope_thread;
  threadCreate(&qtscope_thread, nrgNBQtscopeThread, p->gNB->scopeData, (char*)"qtscope", -1, sched_get_priority_min(SCHED_RR));
}

extern "C" void nrqtscope_autoinit(void *dataptr) {
  AssertFatal( (IS_SOFTMODEM_GNB_BIT||IS_SOFTMODEM_5GUE_BIT),"Scope cannot find NRUE or GNB context");

  if (IS_SOFTMODEM_GNB_BIT)
    nrgNBinitQtScope((scopeParms_t *)dataptr);
  else
    nrUEinitQtScope((PHY_VARS_NR_UE *)dataptr);
}

