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

//qPCP
#include <Partition.h>

//CCCoreLib
#include <CloudSamplingTools.h>

//qCC_db
#include <ccNormalVectors.h>
#include <ccOctree.h>
#include <ccPointCloud.h>
#include <ccPointCloudInterpolator.h>
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
    std::vector<ccPointCloud*> clouds;
    for ( ccHObject* entity : selectedEntities )
    {
        if ( entity && entity->isA( CC_TYPES::POINT_CLOUD ) )
        {
            clouds.push_back( ccHObjectCaster::ToPointCloud(entity) );
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
    bool useRGB = false;

    // some parallel cut pursuit params
	PCP::Parameters params;
    params.knn           = 26;
    params.knnRadius     = resolution;
    params.regularization = dlg.getRegularization();
    params.spatialWeight = 0.00001f;
    params.cutoff        = dlg.getCutoff();

    ccProgressDialog pDlg(parentWidget);
    pDlg.setAutoClose(false);

    ccProgressDialog pOctreeDlg(allowDialogs, parentWidget);
    pOctreeDlg.setAutoClose(false);

    //Duration: initialization
	QElapsedTimer initTimer;
	initTimer.start();

    for (ccPointCloud* cloud : clouds)
	{
        //Duration: volume computation
        QElapsedTimer timer;
        timer.start();

        auto cloudName = cloud->getName();

        ccOctree::Shared theOctree = cloud->getOctree();
        if (!theOctree)
        {
            theOctree = cloud->computeOctree(&pOctreeDlg);
            if (!theOctree)
            {
                app->dispToConsole(QObject::tr("[Facets2] Couldn't compute octree for cloud '%1'!").arg(cloud->getName()), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
                break;
            }
        }

        auto subsampled = CCCoreLib::CloudSamplingTools::resampleCloudSpatially(cloud,
                                                                                static_cast<PointCoordinateType>(resolution),
                                                                                CCCoreLib::CloudSamplingTools::SFModulationParams(false),
                                                                                theOctree.data(),
                                                                                &pDlg);
        if (!subsampled)
        {
            app->dispToConsole(QObject::tr("[Facets2] Failed to subsample cloud '%1'!").arg(cloud->getName()), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
            break;
        }

        ccPointCloud* pc = cloud->partialClone(subsampled);
        delete subsampled;
        subsampled = nullptr;
        if (!pc)
        {
            app->dispToConsole(QObject::tr("[Facets2] Not enough memory to subsample cloud '%1'!").arg(cloud->getName()), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
            break;
        }

        pc->computeNormalsWithOctree(CCCoreLib::LOCAL_MODEL_TYPES::LS, 
                                     ccNormalVectors::Orientation::PLUS_Z,
                                     static_cast<PointCoordinateType>(resolution * RES_FACTOR_NORMALS), 
                                     &pDlg);

        ccOctree::Shared theSubOctree = pc->getOctree();
        if (!theSubOctree)
        {
            theSubOctree = pc->computeOctree(&pOctreeDlg);
            if (!theSubOctree)
            {
                app->dispToConsole(QObject::tr("[Facets2] Couldn't compute octree for cloud '%1'!").arg(pc->getName()), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
                break;
            }
        }

        // we create/activate Facet ID's label scalar field
        int sfIdx = pc->getScalarFieldIndexByName(FACET_SF_NAME);
        if (sfIdx < 0)
        {
            sfIdx = pc->addScalarField(FACET_SF_NAME);
        }
        if (sfIdx < 0)
        {
            app->dispToConsole(QObject::tr("[Facets2] Couldn't allocate a new scalar field for computing Facets! Try to free some memory ..."), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
            break;
        }
        pc->setCurrentScalarField(sfIdx);
        
        // more parallel cut pursuit params
        int32_t  D      = 6; // 3 for XYZ + 3 for Normals (Nx, Ny, Nz)
        int32_t  N      = static_cast<int32_t>(pc->size());

        params.D = D;
		params.N = N;
		params.Y.assign(static_cast<size_t>(N) * static_cast<size_t>(D), 0.0f);
		std::vector<float>&  Y = params.Y;
        std::vector<int32_t> components;

        CCVector3d posOffset(0, 0, 0);
        for (int32_t i = 0; i < N; ++i)
        {
            posOffset += pc->getPoint(i)->toDouble();
        }
        posOffset /= static_cast<double>(N);

        for (int32_t i = 0; i < N; ++i)
        {
            const CCVector3d P = pc->getPoint(i)->toDouble();

            Y[i * D + 0] = static_cast<float>(P.x - posOffset.x);
            Y[i * D + 1] = static_cast<float>(P.y - posOffset.y);
            Y[i * D + 2] = static_cast<float>(P.z - posOffset.z);

            const CCVector3& normal = pc->getPointNormal(i);
            for (size_t k = 0; k < 3; ++k)
            {
                const float n = normal[k];
                //scale normals from [-1,1] to [0,1]
                float value = std::isfinite(n) ? (n + 1.0f) / 2.0f : 0.0f;
                // Scalar fields start at feature index 3
                Y[i * D + 3 + k] =value;
            }
        }

        // we try to label all CCs
        int                  rV = PCP::Partition::labelCutPursuitComponents(pc,
                                                                            params,
                                                                            components,
                                                                            &pDlg,
                                                                            theSubOctree.data());

        // error handling
        if (rV < 0)
        {
            app->dispToConsole(QObject::tr("[Facets2] Failed to compute components!"), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
            pc->deleteScalarField(sfIdx);
            return false;
        }

        // Assign component index to each point
        ccScalarField::Shared sf = pc->getCCScalarField(sfIdx);
        for (int32_t i = 0; i < N; ++i)
        {
            sf->setValue(i, static_cast<ScalarType>(components[i]));
        }
        sf->computeMinAndMax();

        //Upadate display
        cloud->setEnabled(false);

        pc->setName(QObject::tr("%1 (res.:%2 - reg:%2 - cutoff:%5)").arg(cloud->getName()).arg(resolution).arg(params.regularization).arg(params.cutoff));
        pc->setCurrentDisplayedScalarField(sfIdx);
        pc->showSF(true);
        pc->prepareDisplayForRefresh();

        app->addToDB(pc);
        app->refreshAll();

        qint64 time_ms = timer.elapsed();
        //we display block volume computation timing only if no error occurred!
        if (app)
        {
            app->dispToConsole(QObject::tr("[Facets2] Detected %1 facets in cloud '%2': %3 s").arg(rV).arg(cloudName).arg(time_ms / 1000.0, 0, 'f', 3),
                ccMainAppInterface::STD_CONSOLE_MESSAGE);
        }
    }
    
    return true;
}
