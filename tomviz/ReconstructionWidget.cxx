/******************************************************************************
 
 This source file is part of the tomviz project.
 
 Copyright Kitware, Inc.
 
 This source code is released under the New BSD License, (the "License").
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 
 ******************************************************************************/

#include "ReconstructionWidget.h"

#include "ui_ReconstructionWidget.h"

#include "DataSource.h"
#include "LoadDataReaction.h"
#include "TomographyTiltSeries.h"
#include "TomographyReconstruction.h"
#include "Utilities.h"

#include "vtkActor.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageProperty.h"
#include "vtkLineSource.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkScalarsToColors.h"
#include "vtkSMSourceProxy.h"
#include "vtkTrivialProducer.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPointer>
#include <QThread>

namespace tomviz {

class ReconstructionWidget::RWInternal
{
public:
  Ui::ReconstructionWidget Ui;
  vtkNew<vtkImageSliceMapper> dataSliceMapper;
  vtkNew<vtkImageSliceMapper> reconstructionSliceMapper;
  vtkNew<vtkImageSliceMapper> sinogramMapper;
  vtkNew<vtkImageSlice> dataSlice;
  vtkNew<vtkImageData> reconstruction;
  vtkNew<vtkImageSlice> reconstructionSlice;
  vtkNew<vtkImageSlice> sinogram;

  vtkNew<vtkRenderer> dataSliceRenderer;
  vtkNew<vtkRenderer> reconstructionSliceRenderer;
  vtkNew<vtkRenderer> sinogramRenderer;

  vtkNew<vtkLineSource> currentSliceLine;
  vtkNew<vtkActor> currentSliceActor;
  QPointer<DataSource> dataSource;
  bool cancelled;
  bool started;

  void setupCurrentSliceLine(int sliceNum)
  {
    vtkTrivialProducer *t =
        vtkTrivialProducer::SafeDownCast(this->dataSource->producer()->GetClientSideObject());
    if (!t)
    {
      return;
    }
    vtkImageData *imageData = vtkImageData::SafeDownCast(t->GetOutputDataObject(0));
    if (imageData)
    {
      int extent[6];
      imageData->GetExtent(extent);
      double spacing[3];
      imageData->GetSpacing(spacing);
      double bounds[6];
      imageData->GetBounds(bounds);
      double point1[3], point2[3];
      point1[0] = bounds[0] + sliceNum * spacing[0];
      point2[0] = bounds[0] + sliceNum * spacing[0];
      point1[1] = bounds[2] - (bounds[3] - bounds[2]);
      point2[1] = bounds[3] + (bounds[3] - bounds[2]);
      point1[2] = bounds[5] + 1;
      point2[2] = bounds[5] + 1;
      this->currentSliceLine->SetPoint1(point1);
      this->currentSliceLine->SetPoint2(point2);
      this->currentSliceLine->Update();
      this->currentSliceActor->GetMapper()->Update();
    }
  }

};

ReconstructionWidget::ReconstructionWidget(DataSource *source, QWidget *p)
  : QWidget(p), Internals(new RWInternal)
{
  this->Internals->Ui.setupUi(this);
  this->Internals->dataSource = source;
  this->Internals->cancelled = false;
  this->Internals->started = false;

  vtkTrivialProducer *t = vtkTrivialProducer::SafeDownCast(
    source->producer()->GetClientSideObject());

  this->Internals->dataSliceMapper->SetInputConnection(t->GetOutputPort());
  this->Internals->sinogramMapper->SetInputConnection(t->GetOutputPort());
  this->Internals->sinogramMapper->SetOrientationToX();
  this->Internals->sinogramMapper->SetSliceNumber(
    this->Internals->sinogramMapper->GetSliceNumberMinValue());
  this->Internals->sinogramMapper->Update();

  vtkImageData *imageData = vtkImageData::SafeDownCast(t->GetOutputDataObject(0));
  if (!imageData)
  {
    // We can't really handle this well since we depend on there being data
    return;
  }
  int extent[6];
  imageData->GetExtent(extent);
  this->Internals->dataSliceMapper->SetSliceNumber(extent[0] + (extent[1] - extent[0]) / 2);
  this->Internals->dataSliceMapper->Update();
  int extent2[6] = {extent[0], extent[1], extent[2], extent[3], extent[2], extent[3]};
  this->Internals->reconstruction->SetExtent(extent2);
  this->Internals->reconstruction->AllocateScalars(VTK_FLOAT, 1);
  vtkDataArray *darray = this->Internals->reconstruction->GetPointData()->GetScalars();
  darray->FillComponent(0, 0);
  this->Internals->reconstructionSliceMapper->SetInputData(this->Internals->reconstruction.Get());
  this->Internals->reconstructionSliceMapper->SetOrientationToX();
  this->Internals->reconstructionSliceMapper->Update();

  this->Internals->dataSlice->SetMapper(this->Internals->dataSliceMapper.Get());
  this->Internals->reconstructionSlice->SetMapper(
    this->Internals->reconstructionSliceMapper.Get());
  this->Internals->sinogram->SetMapper(this->Internals->sinogramMapper.Get());

  vtkScalarsToColors *lut =
      vtkScalarsToColors::SafeDownCast(source->colorMap()->GetClientSideObject());

  this->Internals->dataSlice->GetProperty()->SetLookupTable(lut);
  this->Internals->reconstructionSlice->GetProperty()->SetLookupTable(lut);
  this->Internals->sinogram->GetProperty()->SetLookupTable(lut);

  this->Internals->dataSliceRenderer->AddViewProp(this->Internals->dataSlice.Get());
  this->Internals->reconstructionSliceRenderer->AddViewProp(
    this->Internals->reconstructionSlice.Get());
  this->Internals->sinogramRenderer->AddViewProp(this->Internals->sinogram.Get());

  this->Internals->currentSliceLine->SetPoint1(0, 0, 0);
  this->Internals->currentSliceLine->SetPoint2(1, 1, 1);
  this->Internals->currentSliceLine->Update();

  vtkNew<vtkPolyDataMapper> lineMapper;
  lineMapper->SetInputConnection(this->Internals->currentSliceLine->GetOutputPort());
  this->Internals->currentSliceActor->SetMapper(lineMapper.Get());
  this->Internals->currentSliceActor->GetProperty()->SetColor(0.8, 0.4, 0.4);
  this->Internals->currentSliceActor->GetProperty()->SetLineWidth(2.5);
  this->Internals->dataSliceRenderer->AddViewProp(
    this->Internals->currentSliceActor.Get());

  this->Internals->Ui.currentSliceView->GetRenderWindow()->AddRenderer(
    this->Internals->dataSliceRenderer.Get());
  this->Internals->Ui.currentReconstructionView->GetRenderWindow()->AddRenderer(
    this->Internals->reconstructionSliceRenderer.Get());
  this->Internals->Ui.sinogramView->GetRenderWindow()->AddRenderer(
    this->Internals->sinogramRenderer.Get());

  tomviz::setupRenderer(this->Internals->dataSliceRenderer.Get(),
                        this->Internals->dataSliceMapper.Get());
  tomviz::setupRenderer(this->Internals->sinogramRenderer.Get(),
                        this->Internals->sinogramMapper.Get());
  tomviz::setupRenderer(this->Internals->reconstructionSliceRenderer.Get(),
                        this->Internals->reconstructionSliceMapper.Get());

  this->connect(this->Internals->Ui.startButton, SIGNAL(pressed()),
                SLOT(startReconstruction()));
  this->connect(this->Internals->Ui.cancelButton, SIGNAL(pressed()),
                SLOT(cancelReconstruction()));
}

ReconstructionWidget::~ReconstructionWidget()
{
  delete this->Internals;
}

void ReconstructionWidget::startReconstruction()
{
  this->Internals->started = true;
  DataSource *source = this->Internals->dataSource;
  Ui::ReconstructionWidget &ui = this->Internals->Ui;
  if (!source)
  {
    return;
  }
  vtkTrivialProducer *t =
    vtkTrivialProducer::SafeDownCast(source->producer()->GetClientSideObject());
  vtkImageData *imageData = vtkImageData::SafeDownCast(t->GetOutputDataObject(0));
  int extent[6];
  imageData->GetExtent(extent);
  int numXSlices = extent[1] - extent[0] + 1;
  int numYSlices = extent[3] - extent[2] + 1;
  int numZSlices = extent[5] - extent[4] + 1;
  ui.statusLabel->setText(QString("Slice # 0 out of %1\nTime remaining: unknown").arg(numXSlices));
  QElapsedTimer timer;
  timer.start();
  std::vector<float> sinogramPtr(numYSlices * numZSlices);
  std::vector<float> reconstructionPtr(numYSlices * numYSlices);
  QVector<double> tiltAngles = source->getTiltAngles();
  vtkDataArray *darray = this->Internals->reconstruction->GetPointData()->GetScalars();
  // TODO: talk to Dave Lonie about how to do this in new data array API
  float *reconstruction = (float*)darray->GetVoidPointer(0);
  for (int i = 0; i < numXSlices && !this->Internals->cancelled; ++i)
  {
    QCoreApplication::processEvents();
    this->Internals->setupCurrentSliceLine(i);
    ui.currentSliceView->GetRenderWindow()->Render();
    this->Internals->sinogramMapper->SetSliceNumber(
      this->Internals->sinogramMapper->GetSliceNumberMinValue() + i);
    ui.sinogramView->GetRenderWindow()->Render();
    TomographyTiltSeries::getSinogram(imageData, i, &sinogramPtr[0]);
    TomographyReconstruction::unweightedBackProjection2(
      &sinogramPtr[0], tiltAngles.data(), &reconstructionPtr[0], numZSlices, numYSlices);
    for (int j = 0; j < numYSlices; ++j) {
      for (int k = 0; k < numYSlices; ++k) {
        reconstruction[j * (numYSlices * numXSlices) + k * numXSlices + i] = reconstructionPtr[k * numYSlices + j];
      }
    }
    this->Internals->reconstruction->Modified();
    this->Internals->reconstructionSliceMapper->SetSliceNumber(i);
    this->Internals->reconstructionSliceMapper->Update();
    ui.currentReconstructionView->GetRenderWindow()->Render();
    ui.statusLabel->setText(QString("Slice # %1 out of %2\nTime remaining: %3 seconds")
        .arg(i+1).arg(numXSlices).arg((timer.elapsed() / (1000.0*(i+1))) * (numXSlices - i)));
  }
  if (!this->Internals->cancelled)
  {
    DataSource* output = source->clone(true,true);
    QString name = output->producer()->GetAnnotation("tomviz.Label");
    name = "Recon_WBP_" + name;
    output->producer()->SetAnnotation("tomviz.Label", name.toLatin1().data());
    t = vtkTrivialProducer::SafeDownCast(output->producer()->GetClientSideObject());
    t->SetOutput(this->Internals->reconstruction.Get());
    output->dataModified();
    LoadDataReaction::dataSourceAdded(output);
    emit this->reconstructionFinished();
  }
  else
  {
    emit this->reconstructionCancelled();
  }
}

void ReconstructionWidget::cancelReconstruction()
{
  if (this->Internals->started)
  {
    this->Internals->cancelled = true;
  }
  else
  {
    emit this->reconstructionCancelled();
  }
}
}
