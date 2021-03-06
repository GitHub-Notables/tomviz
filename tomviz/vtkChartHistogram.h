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
#ifndef tomvizvtkChartHistogram_h
#define tomvizvtkChartHistogram_h

#include <vtkChartXY.h>

#include <vtkNew.h>
#include <vtkTransform2D.h>

class vtkContextMouseEvent;
class vtkCustomPiecewiseControlPointsItem;
class vtkHistogramMarker;
class vtkPiecewiseFunction;
class vtkPiecewiseFunctionItem;
class vtkPlotBar;
class vtkScalarsToColors;
class vtkTable;

class vtkChartHistogram : public vtkChartXY
{
public:
  static vtkChartHistogram* New();

  bool MouseDoubleClickEvent(const vtkContextMouseEvent& mouse) override;

  // Set input for histogram
  virtual void SetHistogramInputData(vtkTable* table, const char* xAxisColumn,
                                     const char* yAxisColumn);

  // Set scalar visibility in the histogram plot bar
  virtual void SetScalarVisibility(bool visible);
  virtual void ScalarVisibilityOn();

  void SetHistogramVisible(bool visible);

  void SetMarkerVisible(bool visible);

  // Set lookup table
  virtual void SetLookupTable(vtkScalarsToColors* lut);

  // Set the color array name
  virtual void SelectColorArray(const char* arrayName);

  // Set opacity function from a transfer function
  virtual void SetOpacityFunction(vtkPiecewiseFunction* opacityFunction);

  // Set the contour value from the contour marker
  vtkSetMacro(ContourValue, double) vtkGetMacro(ContourValue, double)

  // Set the DPI of the chart.
  void SetDPI(int dpi);

protected:
  vtkNew<vtkTransform2D> Transform;
  double ContourValue;
  vtkNew<vtkHistogramMarker> Marker;

  vtkNew<vtkPlotBar> HistogramPlotBar;
  vtkNew<vtkPiecewiseFunctionItem> OpacityFunctionItem;
  vtkNew<vtkCustomPiecewiseControlPointsItem> OpacityControlPointsItem;

private:
  vtkChartHistogram();
  virtual ~vtkChartHistogram();
};

#endif // tomvizvtkChartHistogram_h
