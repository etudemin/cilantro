#include <cilantro/icp_common_instances.hpp>
#include <cilantro/point_cloud.hpp>
#include <cilantro/visualizer.hpp>
#include <cilantro/common_renderables.hpp>
#include <cilantro/timer.hpp>

void color_toggle(cilantro::Visualizer &viz) {
    cilantro::RenderingProperties rp = viz.getRenderingProperties("dst");
    if (rp.pointColor == cilantro::RenderingProperties::noColor) {
        rp.setPointColor(0.0f, 0.0f, 1.0f);
    } else {
        rp.setPointColor(cilantro::RenderingProperties::noColor);
    }
    viz.setRenderingProperties("dst", rp);
    rp = viz.getRenderingProperties("src");
    if (rp.pointColor == cilantro::RenderingProperties::noColor) {
        rp.setPointColor(1.0f, 0.0f, 0.0f);
    } else {
        rp.setPointColor(cilantro::RenderingProperties::noColor);
    }
    viz.setRenderingProperties("src", rp);
}

void warp_toggle(cilantro::Visualizer &viz) {
    viz.toggleVisibility("corr");
}

int main(int argc, char ** argv) {
    if (argc < 3) {
        std::cout << "Please provide paths to two PLY files." << std::endl;
        return 0;
    }

    cilantro::PointCloud3f dst(argv[1]), src(argv[2]);
    if (!dst.hasNormals()) {
        std::cout << "Target point cloud is empty or does not have normals!" << std::endl;
        return 0;
    }


    // Example 1: Compute a sparsely supported warp field (compute transformations for a sparse set of control nodes)
    // Neighborhood parameters
    float control_res = 0.025f;
    float src_to_control_sigma = 0.5f*control_res;
    float regularization_sigma = 3.0f*control_res;

    float max_correspondence_dist_sq = 0.02f*0.02f;

    // Get a sparse set of control nodes by downsampling
    cilantro::VectorSet<float,3> control_points = cilantro::PointsGridDownsampler3f(src.points, control_res).getDownsampledPoints();
    cilantro::KDTree<float,3> control_tree(control_points);

    // Find which control nodes affect each point in src
    cilantro::NeighborhoodSet<float> src_to_control_nn = control_tree.search(src.points, cilantro::KNNNeighborhoodSpecification(4));

    // Get regularization neighborhoods for control nodes
    cilantro::NeighborhoodSet<float> regularization_nn = control_tree.search(control_points, cilantro::KNNNeighborhoodSpecification(12));

    // Perform ICP registration
    cilantro::Timer timer;
    timer.start();

    // for(int i = 0; i < 1; i++)
    // {
    //     Eigen::Vector3f src_p = src.points.col(i);
    //     printf("%d source point: (%f, %f, %f)\n", i, src_p[0], src_p[1], src_p[2]);
        
    //     for(int j = 0; j < 4; j++)
    //     {   
    //         int index = src_to_control_nn[i][j].index;
    //         Eigen::Vector3f ctrl_p = control_points.col(index);
    //         printf("%d KNN: (%f, %f, %f):\n", j, ctrl_p[0], ctrl_p[1], ctrl_p[2]);
    //         printf("index: %d\n", index);
    //         printf("value: %3.12f\n", src_to_control_nn[i][j].value);

    //         float diff[3] = { std::abs(src_p[0] - ctrl_p[0]), std::abs(src_p[1] - ctrl_p[1]), std::abs(src_p[2] - ctrl_p[2]) };
    //         float diff_square = diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2];
    //         printf("sum of diff: %3.12f\n", diff[0] + diff[1] + diff[2]);
    //         printf("sum of square of diff: %3.12f\n", diff_square);
    //         printf("diff: %3.12f\n", std::sqrt(diff_square));
    //         printf("\n");
    //     }
    //     printf("\n");
    // }

//    cilantro::SimpleCombinedMetricSparseAffineWarpFieldICP3f icp(dst.points, dst.normals, src.points, src_to_control_nn, control_points.cols(), regularization_nn);
    // cilantro::SimpleCombinedMetricSparseRigidWarpFieldICP3f icp(dst.points, dst.normals, src.points, src_to_control_nn, control_points.cols(), regularization_nn);
    cilantro::SimpleCombinedMetricSparseRigidWarpFieldICP3f icp(dst.points, dst.normals, src.points, control_points, src_to_control_nn, control_points.cols(), regularization_nn);

    // Parameter setting
    icp.correspondenceSearchEngine().setMaxDistance(max_correspondence_dist_sq);
    icp.controlWeightEvaluator().setSigma(src_to_control_sigma);
    icp.regularizationWeightEvaluator().setSigma(regularization_sigma);

    icp.setMaxNumberOfIterations(15).setConvergenceTolerance(2.5e-5f);
    icp.setMaxNumberOfGaussNewtonIterations(1).setGaussNewtonConvergenceTolerance(5e-4f);
    icp.setMaxNumberOfConjugateGradientIterations(500).setConjugateGradientConvergenceTolerance(1e-5f);
    icp.setPointToPointMetricWeight(0.0f).setPointToPlaneMetricWeight(1.0f).setStiffnessRegularizationWeight(200.0f);
    icp.setHuberLossBoundary(1e-2f);

    auto tf_est = icp.estimate().getDenseWarpField();

    timer.stop();


//    // Example 2: Compute a densely supported warp field (each point has its own local transformation)
//    float res = 0.005f;
//    float regularization_sigma = 3.0f*res;
//
//    dst.gridDownsample(res).removeInvalidData();
//    src.gridDownsample(res).removeInvalidData();
//
//    float max_correspondence_dist_sq = 0.04f*0.04f;
//
//    std::vector<cilantro::NeighborSet<float>> regularization_nn;
//    cilantro::KDTree3f(src.points).search(src.points, cilantro::KNNNeighborhoodSpecification(12), regularization_nn);
//
//    // Perform ICP registration
//    cilantro::Timer timer;
//    timer.start();
//
////    cilantro::SimpleCombinedMetricDenseAffineWarpFieldICP3f icp(dst.points, dst.normals, src.points, regularization_nn);
//    cilantro::SimpleCombinedMetricDenseRigidWarpFieldICP3f icp(dst.points, dst.normals, src.points, regularization_nn);
//
//    // Parameter setting
//    icp.correspondenceSearchEngine().setMaxDistance(max_correspondence_dist_sq);
//    icp.regularizationWeightEvaluator().setSigma(regularization_sigma);
//
//    icp.setMaxNumberOfIterations(15).setConvergenceTolerance(2.5e-3f);
//    icp.setMaxNumberOfGaussNewtonIterations(1).setGaussNewtonConvergenceTolerance(5e-4f);
//    icp.setMaxNumberOfConjugateGradientIterations(500).setConjugateGradientConvergenceTolerance(1e-5f);
//    icp.setPointToPointMetricWeight(0.1f).setPointToPlaneMetricWeight(1.0f).setStiffnessRegularizationWeight(200.0f);
//    icp.setHuberLossBoundary(1e-2f);
//
//    auto tf_est = icp.estimate().getTransform();
//
//    timer.stop();


    std::cout << "Registration time: " << timer.getElapsedTime() << "ms" << std::endl;
    std::cout << "Iterations performed: " << icp.getNumberOfPerformedIterations() << std::endl;
    std::cout << "Has converged: " << icp.hasConverged() << std::endl;

    timer.start();
    auto residuals = icp.getResiduals();
    timer.stop();
    std::cout << "Residual computation time: " << timer.getElapsedTime() << "ms" << std::endl;

    // Visualization
    pangolin::CreateWindowAndBind("Rigid ICP example", 1920, 480);
    pangolin::Display("multi").SetBounds(0.0, 1.0, 0.0, 1.0).SetLayout(pangolin::LayoutEqual)
            .AddDisplay(pangolin::Display("initial"))
            .AddDisplay(pangolin::Display("registration"))
            .AddDisplay(pangolin::Display("residuals"));

    cilantro::Visualizer initial_and_warp_viz("Rigid ICP example", "initial");
    cilantro::Visualizer registration_viz("Rigid ICP example", "registration");
    cilantro::Visualizer residuals_viz("Rigid ICP example", "residuals");

    // Warp src
    auto warped = src.transformed(tf_est);

    // Initial state and warp field
    initial_and_warp_viz.registerKeyboardCallback('c', std::bind(color_toggle, std::ref(initial_and_warp_viz)));
    initial_and_warp_viz.registerKeyboardCallback('w', std::bind(warp_toggle, std::ref(initial_and_warp_viz)));
    initial_and_warp_viz.addObject<cilantro::PointCloudRenderable>("dst", dst, cilantro::RenderingProperties().setPointColor(0,0,1));
    initial_and_warp_viz.addObject<cilantro::PointCloudRenderable>("src", src, cilantro::RenderingProperties().setPointColor(1,0,0));
    initial_and_warp_viz.addObject<cilantro::PointCorrespondencesRenderable>("corr", warped, src, cilantro::RenderingProperties().setLineWidth(0.1f));

    // Registration result
    registration_viz.registerKeyboardCallback('c', std::bind(color_toggle, std::ref(registration_viz)));
    registration_viz.addObject<cilantro::PointCloudRenderable>("dst", dst, cilantro::RenderingProperties().setPointColor(0,0,1).setPointSize(1.0f));
    registration_viz.addObject<cilantro::PointCloudRenderable>("src", warped, cilantro::RenderingProperties().setPointColor(1,0,0).setPointSize(1.0f));

    // Residuals
    residuals_viz.addObject<cilantro::PointCloudRenderable>("src", warped, cilantro::RenderingProperties().setUseLighting(false))
            ->setPointValues(residuals);

    std::cout << "Press 'c' to toggle point cloud colors" << std::endl;
    std::cout << "Press 'w' to toggle warp field visibility" << std::endl;
    while (!initial_and_warp_viz.wasStopped() && !registration_viz.wasStopped() && !residuals_viz.wasStopped()) {
        initial_and_warp_viz.clearRenderArea();
        initial_and_warp_viz.render();
        registration_viz.render();
        residuals_viz.render();
        pangolin::FinishFrame();
    }

    return 0;
}