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
#include "ResetReaction.h"

#include "ModuleManager.h"

namespace tomviz {

ResetReaction::ResetReaction(QAction* parentObject) : Superclass(parentObject)
{}

void ResetReaction::updateEnableState()
{
  bool enabled = !ModuleManager::instance().hasRunningOperators();
  parentAction()->setEnabled(enabled);
}

void ResetReaction::reset()
{
  ModuleManager::instance().reset();
}
} // namespace tomviz
