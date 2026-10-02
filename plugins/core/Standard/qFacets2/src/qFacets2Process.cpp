//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: qFacets2                          #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                             COPYRIGHT: Ioannis Farmakis                #
//#                                                                        #
//##########################################################################

#include "qFacets2Process.h"

//local
#include "qFacets2Dialog.h"

//qCC_plugins
#include <ccMainAppInterface.h>

//qCC_pcp
#include <Partition.h>

//qCC_db
#include <ccNormalVectors.h>
#include <ccOctree.h>
#include <ccPointCloud.h>
#include <ccProgressDialog.h>
#include <ccScalarField.h>

//Qt
#include <QElapsedTimer>
#include <QMessageBox>

//System
#include <cmath>

using namespace PCP;

int RES_FACTOR_NORMALS = 3; // resolution factor for normals computation 
//! Default name for Facets2 scalar fields
static const char FACET_SF_NAME[] = "Facet ID";


bool qFacets2Process::Compute(const qFacets2Dialog& dlg,
                                QString& errorMessage,
                                ccPointCloud*& outputCloud,
                                ccHObject*& outputGroup,
                                bool allowDialogs,
                                QWidget* parentWidget,
                                ccMainAppInterface* app)
{
    //get the selected entities
    const ccHObject::Container& selectedEntities = app->getSelectedEntities();

    //check if there is at least one point cloud in the selection
    ccHObject::Container clouds;
    for ( ccHObject* entity : selectedEntities )
    {
        if ( entity && entity->isKindOf( CC_TYPES::POINT_CLOUD ) )
        {
            clouds.push_back( entity );
        }
    }

    if ( clouds.empty() )
    {
        errorMessage = "No point cloud has been selected!";
        return false;
    }

    // get the parameters from the dialog
    double resolution = dlg.getResolution();
    double minPlanarity = dlg.getMinPlanarity();
    float regularization = static_cast<float>(dlg.getRegularization());
    int32_t cutoff = dlg.getCutoff();
    // more parameteres (hard-coded)
    int32_t knn = 26;
    float spatialWeight = 0.00001f;
    bool useRGB = false;

    ccProgressDialog pDlg(parentWidget);

    //Duration: initialization
	QElapsedTimer initTimer;
	initTimer.start();

    for (ccHObject* entity : clouds)
	{
        if (entity && entity->isA(CC_TYPES::POINT_CLOUD))
        {
            ccPointCloud* pc = static_cast<ccPointCloud*>(entity);

            ccOctree::Shared theOctree = pc->getOctree();
            if (!theOctree)
            {
                ccProgressDialog pOctreeDlg(allowDialogs, parentWidget);
                theOctree = pc->computeOctree(&pOctreeDlg);
                if (!theOctree)
                {
                    app->dispToConsole(QObject::tr("Couldn't compute octree for cloud '%1'!").arg(pc->getName()), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
                    break;
                }
            }
            auto pc_sub = CCCoreLib::CloudSamplingTools::resampleCloudSpatially(cloud,
                                                                            static_cast<PointCoordinateType>(resolution),
                                                                            CCCoreLib::SFModulationParams(),
                                                                            theOctree.data(),
                                                                            &pDlg);

            pc_sub->computeNormalsWithOctree(CCCoreLib::LOCAL_MODEL_TYPES::LS, 
                                        ccNormalVectors::Orientation::PLUS_Z,
                                        static_cast<PointCoordinateType>(resolution * RES_FACTOR_NORMALS), 
                                        &pDlg);

            // we create/activate Facet ID's label scalar field
            int sfIdx = pc_sub->getScalarFieldIndexByName(FACET_SF_NAME);
            if (sfIdx < 0)
            {
                sfIdx = pc_sub->addScalarField(FACET_SF_NAME);
            }
            if (sfIdx < 0)
            {
                app->dispToConsole(QObject::tr("Couldn't allocate a new scalar field for computing Facets! Try to free some memory ..."), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
                break;
            }
            pc_sub->setCurrentScalarField(sfIdx);

            
            // some parallel cut pursuit params
            int32_t            D      = 6; // 3 for XYZ + 3 for Normals (Nx, Ny, Nz)
            int32_t            N      = static_cast<int32_t>(pc_sub->size());
            std::vector<float> Y(static_cast<size_t>(N) * static_cast<size_t>(D), 0.0f);

            CCVector3 posOffset(0, 0, 0);
            for (int32_t i = 0; i < N; ++i)
            {
                posOffset += *pc_sub->getPoint(i);
            }
            posOffset /= static_cast<float>(N);

            for (int32_t i = 0; i < N; ++i)
            {
                const CCVector3* P = pc_sub->getPoint(i);

                Y[i * D + 0] = static_cast<float>(P->x - posOffset.x);
                Y[i * D + 1] = static_cast<float>(P->y - posOffset.y);
                Y[i * D + 2] = static_cast<float>(P->z - posOffset.z);

                auto sfIdx->getScalarFieldIndexByName("Nx")
                ccScalarField::Shared sf    = std::static_pointer_cast<ccScalarField>(pc_sub->getScalarField(sfIdx));
                float                 value = static_cast<float>(sf->getValue(i));

                auto sfIdx->getScalarFieldIndexByName("Ny")
                ccScalarField::Shared sf    = std::static_pointer_cast<ccScalarField>(pc_sub->getScalarField(sfIdx));
                float                 value = static_cast<float>(sf->getValue(i));

                auto sfIdx->getScalarFieldIndexByName("Nz")
                ccScalarField::Shared sf    = std::static_pointer_cast<ccScalarField>(pc_sub->getScalarField(sfIdx));
                float                 value = static_cast<float>(sf->getValue(i));

                for (size_t k = 0; k < 3; ++k)
                {
                    ccScalarField::Shared sf    = std::static_pointer_cast<ccScalarField>(pc_sub->getScalarField(sfIndices[k]));
                    float                 value = static_cast<float>(sf->getValue(i));

                    // Sanitize NaN/Inf, force it to 0.0
                    if (std::isnan(value) || std::isinf(value))
                    {
                        value = 0.0f;
                    }
                    // Scalar fields start at feature index 3, if RGB is used, they start at feature index 6
                    Y[i * D + 3 + rgbDim + k] = value;
                }
            }

            // we try to label all CCs
            std::vector<int32_t> components;
            int                  rV = PCP::Partition::labelCutPursuitComponents(pc_sub,
                                                                knn,
                                                                resolution, // knnRadius
                                                                N,
                                                                D,
                                                                Y,
                                                                regularization,
                                                                spatialWeight,
                                                                cutoff,
                                                                components,
                                                                &pDlg,
                                                                theOctree.data());

            // error handling
            if (rV < 0)
            {
                app->dispToConsole(QObject::tr("[Cut Pursuit] Failed to compute components!"), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
                return false;
            }

            // Assign component index to each point
            ccScalarField::Shared sf = std::static_pointer_cast<ccScalarField>(pc->getScalarField(sfIdx));
            for (int32_t i = 0; i < N; ++i)
            {
                sf->setValue(i, static_cast<ScalarType>(components[i]));
            }
            sf->computeMinAndMax();
        }
    }
    
    return true;
}
