#if !defined(__CINT__) || defined(__MAKECINT__)

 #include <iostream>
 #include <fstream>
 #include <sstream>
 #include <regex>
 #include <string>
 #include <vector>
 #include <cmath>
 #include <map>
 #include <iomanip>
 #include <array>
 #include "stdlib.h"
 #include "TGraphErrors.h"
 #include "TCanvas.h"
 #include "TColor.h"
 #include "TLegend.h"
 #include "TAxis.h"
 #include "TString.h"
 #include "TObjArray.h"
 #include "TObjString.h"
 #include "TF1.h"
 #include "TGaxis.h"
 #include "TPad.h"
 #include "TLatex.h"
 #include "TStyle.h"
 #include "TProfile2D.h"
 #include <limits>
 #include <algorithm>

#endif


constexpr int kMaxTubes = 4;
struct ChannelInfo {
    std::string moduleType;
    std::string moduleSuffix;
    std::string layer;
    std::string moduleNumber;
    int psChannel;
    std::string rate;
    std::string histogram_name;
    double xmin_detector;
    double xmax_detector;
};

Int_t qt_muon_multi(std::string fname = "qscanner_out.2024-10-30_11-38.dat")
{
    std::ifstream infile(fname);
    std::string line, date;
    
    double xmin, xmax, ymin, ymax;
    int xnbins, ynbins, xstep, ystep;
    double scanspeed, standbytime;
    double xcollimatorsize, ycollimatorsize;
    std::vector<double> hv_values;
    
    std::map<int, ChannelInfo> channels;
    bool pattern_found = false;

    std::map<int, double> hv_setpoints;   // PS channel -> HV setpoint (V)
    std::map<int, double> hv_isetpoints;  // PS channel -> I setpoint (uA)

    std::array<std::string, kMaxTubes> xray_sn;
    std::array<double, kMaxTubes> xray_vset_kv;
    std::array<double, kMaxTubes> xray_iset_ua;

    for (int t = 0; t < kMaxTubes; ++t) {
    xray_sn[t] = "";
    xray_vset_kv[t] = std::numeric_limits<double>::quiet_NaN();
    xray_iset_ua[t] = std::numeric_limits<double>::quiet_NaN();
    }

    
    // Read header until we hit data
    std::streampos data_start_pos;
    bool header_complete = false;
    
    while (std::getline(infile, line)) {
        // Check if this is a data line
        std::istringstream test_stream(line);
        double test_val;
        if (test_stream >> test_val) {
            data_start_pos = infile.tellg();
            data_start_pos -= (line.length() + 1);
            header_complete = true;
            break;
        }
        
        // Parse date
        std::regex date_regex(R"((\w+, \d{1,2} \w+ \d{4} \d{2}:\d{2}:\d{2}))");
        std::smatch match;
        if (std::regex_search(line, match, date_regex)) {
            date = match[1];
            std::cout << "Date: " << date << std::endl;
        }
        
        // Parse channel information
        std::regex pattern(R"(# (T11Fs|EIL4(?:B|F|Bs|Fs)|T[1-9](?:B[123]|F[123]|F1Hole|B1s)) # layer # (\d+) # module # (\d+) # PS ch # (\d+) # rate # (\w+))");
        //std::regex pattern(R"(# (T11|EIL4|T#B|T#F|M1|M2|M3|Bs|Fs-L1|Fs-L2|Fs-L3|T11Fs)([A-Z]?) # layer # (\d+) # module # (\d+) # PS ch # (\d+) # rate # (\w+))");
        std::smatch matches;
        
        if (std::regex_search(line, matches, pattern)) {
            ChannelInfo ch;
            ch.moduleType   = matches[1].str();      // detector name
            ch.moduleSuffix = "";                    // not used anymore
            ch.layer        = matches[2].str();
            ch.moduleNumber = matches[3].str();
            ch.psChannel    = std::stoi(matches[4].str());  
            ch.rate         = matches[5].str();      // "low"/"high"

            ch.histogram_name = ch.moduleType + "-L" + ch.layer + "-Module-" + ch.moduleNumber;

            channels[ch.psChannel] = ch;
            pattern_found = true;
            
            std::cout << "Found channel " << ch.psChannel << ": " << ch.histogram_name 
                      << " (rate: " << ch.rate << ")" << std::endl;
        }
        
        // Parse HV values
        // Parse HV setpoints like: <>HV_CH2_VSet 3200 (V)

        // --- HV setpoints: <>HV_CH2_VSet 3200 (V), <>HV_CH2_ISet 300 (uA)
{
    static const std::regex hvv_re(R"(^<>HV_CH(\d+)_VSet\s+([0-9]*\.?[0-9]+))");
    static const std::regex hvi_re(R"(^<>HV_CH(\d+)_ISet\s+([0-9]*\.?[0-9]+))");
    std::smatch m;

    if (std::regex_search(line, m, hvv_re)) {
        hv_setpoints[std::stoi(m[1])] = std::stod(m[2]);
    }
    if (std::regex_search(line, m, hvi_re)) {
        hv_isetpoints[std::stoi(m[1])] = std::stod(m[2]);
    }
}

// --- X-ray SN: <>XRay1_SN 1  , <>XRay2_SN SIM_MODE
{
    static const std::regex xsn_re(R"(^<>XRay(\d+)_SN\s+(\S+))");
    std::smatch m;
    if (std::regex_search(line, m, xsn_re)) {
        int t = std::stoi(m[1]) - 1; // 1-based -> 0-based
        if (t >= 0 && t < kMaxTubes) xray_sn[t] = m[2].str();
    }
}

// --- X-ray setpoints: <>XRay1_VSet 50 (kV), <>XRay1_ISet 75 (uA)
{
    static const std::regex xv_re(R"(^<>XRay(\d+)_VSet\s+([0-9]*\.?[0-9]+))");
    static const std::regex xi_re(R"(^<>XRay(\d+)_ISet\s+([0-9]*\.?[0-9]+))");
    std::smatch m;

    if (std::regex_search(line, m, xv_re)) {
        int t = std::stoi(m[1]) - 1;
        if (t >= 0 && t < kMaxTubes) xray_vset_kv[t] = std::stod(m[2]);
    }
    if (std::regex_search(line, m, xi_re)) {
        int t = std::stoi(m[1]) - 1;
        if (t >= 0 && t < kMaxTubes) xray_iset_ua[t] = std::stod(m[2]);
    }
}


        
        // Parse scan parameters
        if (line.find("XMIN") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            xmin = std::stod(value);
        }
        if (line.find("XMAX") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            xmax = std::stod(value);
        }
        if (line.find("YMIN") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            ymin = std::stod(value);
        }
        if (line.find("YMAX") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            ymax = std::stod(value);
        }
        if (line.find("NBINSX") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            xnbins = std::stoi(value);
        }
        if (line.find("NBINSY") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            ynbins = std::stoi(value);
        }
        if (line.find("XSTEP") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            xstep = std::stoi(value);
        }
        if (line.find("YSTEP") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            ystep = std::stoi(value);
        }
        if (line.find("ScanSpeed") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            scanspeed = std::stod(value);
        }
        if (line.find("StandbyTime") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            standbytime = std::stod(value);
        }
        if (line.find("XCollimatorSize") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            xcollimatorsize = std::stoi(value);
        }
        if (line.find("YCollimatorSize") != std::string::npos) {
            std::istringstream iss(line);
            std::string keyword, value;
            iss >> keyword >> value;
            ycollimatorsize = std::stoi(value);
        }
    }

    if (!pattern_found) {
        std::cout << "Warning: No matching pattern found in the header." << std::endl;
    std::cout << "/***"<< std::endl;
    std::cout << "*    ▗▄▄▖ ▗▞▀▜▌   ■     ■  ▗▞▀▚▖ ▄▄▄ ▄▄▄▄      ▄▄▄▄   ▄▄▄     ■      ▗▞▀▀▘▄▄▄  █  ▐▌▄▄▄▄     ▐▌    ▄ ▄▄▄▄         ■  ▐▌   ▗▞▀▚▖    ▐▌   ▗▞▀▚▖▗▞▀▜▌▐▌▗▞▀▚▖ ▄▄▄ "<< std::endl;
    std::cout << "*    ▐▌ ▐▌▝▚▄▟▌▗▄▟▙▄▖▗▄▟▙▄▖▐▛▀▀▘█    █   █     █   █ █   █ ▗▄▟▙▄▖    ▐▌  █   █ ▀▄▄▞▘█   █    ▐▌    ▄ █   █     ▗▄▟▙▄▖▐▌   ▐▛▀▀▘    ▐▌   ▐▛▀▀▘▝▚▄▟▌▐▌▐▛▀▀▘█    "<< std::endl;
    std::cout << "*    ▐▛▀▘        ▐▌    ▐▌  ▝▚▄▄▖█    █   █     █   █ ▀▄▄▄▀   ▐▌      ▐▛▀▘▀▄▄▄▀      █   █ ▗▞▀▜▌    █ █   █       ▐▌  ▐▛▀▚▖▝▚▄▄▖    ▐▛▀▚▖▝▚▄▄▖  ▗▞▀▜▌▝▚▄▄▖█    "<< std::endl;
     std::cout <<"*    ▐▌          ▐▌    ▐▌                                    ▐▌      ▐▌                   ▝▚▄▟▌    █             ▐▌  ▐▌ ▐▌         ▐▌ ▐▌       ▝▚▄▟▌          "<< std::endl;
     std::cout <<"*                ▐▌    ▐▌                                    ▐▌                                                  ▐▌                                           "<< std::endl;
     std::cout <<"*                                                                                                                                                             "<< std::endl;
     std::cout <<"*                                                                                                                                                             "<< std::endl;
    std::cout << "*/"<< std::endl;
        return 0;
    }
    
    if (channels.empty()) {
        std::cout << "Warning: No channel information found in header!" << std::endl;
        return 0;
    }
    
    int num_channels = channels.size();
    
    // Map channel numbers to column indices
    std::map<int, int> channel_to_column = {{0, 0}, {1, 1}, {2, 2}, {3, 3}};
    
        // Prepare data structures
    std::map<int, TProfile2D*> profiles;
    std::map<int, std::vector<double>> current_data;
    std::map<int, std::vector<double>> x_coords;
    std::map<int, std::vector<double>> voltage_data;  // Store all voltage values
    std::map<int, std::vector<double>> xray_voltage_data;  // X-ray voltage per channel
    std::map<int, std::vector<double>> xray_current_data;  // X-ray current per channel
    
    for (auto& ch_pair : channels) {
        int ch_num = ch_pair.first;
        ChannelInfo& ch = ch_pair.second;
        
        std::string profile_name = Form("IvsXY_ch%d", ch_num);
        std::string profile_title = Form("X-ray scan Channel %d", ch_num);
        
        profiles[ch_num] = new TProfile2D(profile_name.c_str(), profile_title.c_str(),
                                          xnbins, xmin, xmax,
                                          ynbins, ymin, ymax, 
                                          0, 0);
    }
    
    // Read all data
    infile.clear();
    infile.seekg(data_start_pos);

    std::array<std::vector<double>, kMaxTubes> xray_v, xray_i, xray_t;
    
    while (std::getline(infile, line)) {
    std::istringstream iss(line);

    double x, y, Np;
    double i1, i2, i3, i4;
    double v1, v2, v3, v4;

    if (!(iss >> x >> y >> Np >> i1 >> i2 >> i3 >> i4 >> v1 >> v2 >> v3 >> v4)) {
        continue;
    }

    // Collect any extra numbers (0..12) for X-ray tubes: (V I T) repeated
    std::vector<double> extra;
    double tmp;
    while (iss >> tmp) extra.push_back(tmp);

    int nTubesHere = std::min((int)extra.size() / 3, kMaxTubes);
    for (int t = 0; t < nTubesHere; ++t) {
        xray_v[t].push_back(extra[3*t + 0]);
        xray_i[t].push_back(extra[3*t + 1]);
        xray_t[t].push_back(extra[3*t + 2]);
    }

    std::vector<double> currents = {i1, i2, i3, i4};
    std::vector<double> volts    = {v1, v2, v3, v4};

    for (auto& ch_pair : channels) {
        int ch_num = ch_pair.first;

        auto it = channel_to_column.find(ch_num);
        if (it == channel_to_column.end()) continue;
        int col_idx = it->second;

        profiles[ch_num]->Fill(x, y, currents[col_idx]);
        current_data[ch_num].push_back(currents[col_idx]);
        x_coords[ch_num].push_back(x);
        voltage_data[ch_num].push_back(volts[col_idx]);
    }
}
    
    // Analyze data to find active X regions (where detector is located)
    for (auto& ch_pair : channels) {
        int ch_num = ch_pair.first;
        ChannelInfo& ch = ch_pair.second;
        
        // Sort current data to find percentiles
        std::vector<double> sorted_currents = current_data[ch_num];
        std::sort(sorted_currents.begin(), sorted_currents.end());
        
        // Use 75th percentile as threshold - this captures the main detector signal
        size_t idx_75 = sorted_currents.size() * 3 / 4;
        double threshold = sorted_currents[idx_75];
        
        std::cout << "Channel " << ch_num << " - 75th percentile threshold: " << threshold << " nA" << std::endl;
        
        double min_x_active = xmax;
        double max_x_active = xmin;
        
        for (size_t i = 0; i < current_data[ch_num].size(); i++) {
            if (current_data[ch_num][i] > threshold) {
                if (x_coords[ch_num][i] < min_x_active) min_x_active = x_coords[ch_num][i];
                if (x_coords[ch_num][i] > max_x_active) max_x_active = x_coords[ch_num][i];
            }
        }
        
        ch.xmin_detector = min_x_active;
        ch.xmax_detector = max_x_active;
        
        std::cout << "Channel " << ch_num << " (Layer " << ch.layer 
                  << ") active X range: [" << ch.xmin_detector 
                  << ", " << ch.xmax_detector << "]" << std::endl;
    }
    
    // Process each channel
    gStyle->SetPadRightMargin(0.15);
    gStyle->SetPadLeftMargin(0.15);
    
    for (auto& ch_pair : channels) {
        int ch_num = ch_pair.first;
        ChannelInfo& ch = ch_pair.second;
        
        std::cout << "\nProcessing channel " << ch_num << ": " << ch.histogram_name << std::endl;
        
        // Calculate voltage statistics for this channel
        double voltage_mean = 0.0;
        double voltage_std = 0.0;
        
        if (!voltage_data[ch_num].empty()) {
            // Calculate mean
            for (double v : voltage_data[ch_num]) {
                voltage_mean += v;
            }
            voltage_mean /= voltage_data[ch_num].size();
            
            // Calculate standard deviation
            double variance = 0.0;
            for (double v : voltage_data[ch_num]) {
                variance += (v - voltage_mean) * (v - voltage_mean);
            }
            variance /= voltage_data[ch_num].size();
            voltage_std = sqrt(variance);
            
            std::cout << "Voltage statistics - Mean: " << voltage_mean 
                      << " V, Std: " << voltage_std << " V" << std::endl;
        }
std::array<double, kMaxTubes> xray_temp_mean_C;
for (int t = 0; t < kMaxTubes; ++t) {
    if (xray_t[t].empty()) {
        xray_temp_mean_C[t] = std::numeric_limits<double>::quiet_NaN();
    } else {
        double sum = 0.0;
        for (double val : xray_t[t]) sum += val;
        xray_temp_mean_C[t] = sum / xray_t[t].size();
    }
}

std::string voltage_quality = "unknown";
double hv_setpoint = std::numeric_limits<double>::quiet_NaN();
double hv_tolerance = std::numeric_limits<double>::quiet_NaN();
double hv_diff = std::numeric_limits<double>::quiet_NaN();

auto itHV = hv_setpoints.find(ch_num);
if (itHV != hv_setpoints.end()) {
    hv_setpoint = itHV->second;
    hv_tolerance = hv_setpoint * 0.001;  // 0.1%
    hv_diff = std::abs(voltage_mean - hv_setpoint);
    voltage_quality = (hv_diff <= hv_tolerance) ? "good" : "bad";
}

        
        // Create histogram using data from active X region
        std::vector<double> data_in_range;
        for (size_t i = 0; i < current_data[ch_num].size(); i++) {
            if (x_coords[ch_num][i] >= ch.xmin_detector && 
                x_coords[ch_num][i] <= ch.xmax_detector) {
                data_in_range.push_back(current_data[ch_num][i]);
            }
        }
        
        std::cout << "Data points in active X range: " << data_in_range.size() << std::endl;
        
        if (data_in_range.empty()) {
            std::cout << "Warning: No data in X range for channel " << ch_num << std::endl;
            continue;
        }
        
        // Find min/max
        double minI = *std::min_element(data_in_range.begin(), data_in_range.end());
        double maxI = *std::max_element(data_in_range.begin(), data_in_range.end());

        std::cout << "Data range: [" << minI << ", " << maxI << "] nA" << std::endl;
        
// Create histogram with appropriate range based on rate
TH1D *hist;
if (ch.rate == "low") {
    // For low rate: use fixed range 600-900 nA with 200 bins
    hist = new TH1D(Form("I_ch%d", ch_num), "Current", 200, 600, 900);
    std::cout << "Using LOW rate histogram: 200 bins, range [600, 900] nA" << std::endl;
} else {
    // For high rate: use dynamic range with automatic binning
    int nbins = std::max(50, std::min(200, static_cast<int>(1 + 3.322 * log(data_in_range.size()))));
    hist = new TH1D(Form("I_ch%d", ch_num), "Current", nbins, minI, maxI);
    std::cout << "Using HIGH rate histogram: " << nbins << " bins, range [" 
              << minI << ", " << maxI << "] nA" << std::endl;
}

// Fill histogram
for (double val : data_in_range) {
    hist->Fill(val);
}

std::cout << "Histogram filled with " << data_in_range.size() << " entries" << std::endl;
        // Create canvas
        TCanvas *c1 = new TCanvas(Form("c1_ch%d", ch_num), "Profiles", 1000, 800);
        gStyle->SetOptStat(11);
        c1->Divide(1, 2);
        c1->SetFillColor(0);
        c1->SetBorderMode(0);
        c1->SetBorderSize(4);
        c1->SetFrameBorderMode(0);
        
        // Draw 2D profile
        c1->cd(1);
        profiles[ch_num]->Draw("COLZ");
        profiles[ch_num]->GetXaxis()->SetTitle("X coordinate (mm)");
        profiles[ch_num]->GetYaxis()->SetTitle("Y coordinate (mm)");
        profiles[ch_num]->GetZaxis()->SetTitle("Current (nA)");
        
        TLatex *label = new TLatex();
        label->SetNDC();
        label->SetTextSize(0.04);
        label->DrawLatex(0.17, 0.92, Form("Layer %s, Module %s, Ch %d", 
                         ch.layer.c_str(), ch.moduleNumber.c_str(), ch_num));
        
        // Draw histogram
        c1->cd(2);
        gPad->SetGridx();
        gPad->SetGridy();
        hist->SetStats(0);
        hist->Draw("hist");
        hist->SetFillColor(kYellow);
        
        double fit_mean = std::numeric_limits<double>::quiet_NaN();
        double fit_sigma = std::numeric_limits<double>::quiet_NaN();
        
        if (ch.rate == "low") {
            // Find peak position
            int max_bin = hist->GetMaximumBin();
            double peak_pos = hist->GetBinCenter(max_bin);
            double peak_height = hist->GetBinContent(max_bin);
            
            std::cout << "Peak at bin " << max_bin << ", position " << peak_pos 
                      << " nA, height " << peak_height << std::endl;
            
            // Get RMS for initial estimate
            double rms = hist->GetRMS();
            std::cout << "Histogram RMS: " << rms << std::endl;
            
            // Initial fit range: peak ± 2.5 * RMS
            double fit_low = peak_pos - 2.5 * rms;
            double fit_high = peak_pos + 2.5 * rms;
            
            // Make sure range is within histogram bounds
            if (fit_low < minI) fit_low = minI;
            if (fit_high > maxI) fit_high = maxI;
            
            std::cout << "Initial fit range: [" << fit_low << ", " << fit_high << "]" << std::endl;
            
            TF1 *gauss = new TF1(Form("gauss_ch%d", ch_num), "gaus", fit_low, fit_high);
            gauss->SetParameters(peak_height, peak_pos, rms);
            gauss->SetParNames("Constant", "Mean", "Sigma");
            
            // First fit with initial range
            int fit_status = hist->Fit(gauss, "RQ0");
            
            if (fit_status == 0) {
                // Get fitted parameters
                double fitted_mean = gauss->GetParameter(1);
                double fitted_sigma = gauss->GetParameter(2);
                
                std::cout << "First fit - Mean: " << fitted_mean << ", Sigma: " << fitted_sigma << std::endl;
                
                // Refine range to fitted_mean ± 2.5 * fitted_sigma
                fit_low = fitted_mean - 2.5 * fitted_sigma;
                fit_high = fitted_mean + 2.5 * fitted_sigma;
                
                // Make sure range is within histogram bounds
                if (fit_low < minI) fit_low = minI;
                if (fit_high > maxI) fit_high = maxI;
                
                std::cout << "Refined fit range: [" << fit_low << ", " << fit_high << "]" << std::endl;
                
                // Update function range and refit
                gauss->SetRange(fit_low, fit_high);
                fit_status = hist->Fit(gauss, "RQ");
                
                if (fit_status == 0) {
                    double mean = gauss->GetParameter(1);
                    double sigma = gauss->GetParameter(2);
                    double mean_err = gauss->GetParError(1);
                    double sigma_err = gauss->GetParError(2);
                    double ratio = sigma / mean;
                    double ratio_err = ratio * sqrt(pow(sigma_err / sigma, 2) + pow(mean_err / mean, 2));
                    
                    fit_mean = mean;
                    fit_sigma = sigma;
                    
                    std::cout << "Final fit - Mean: " << mean << " ± " << mean_err 
                              << ", Sigma: " << sigma << " ± " << sigma_err << std::endl;
                    
                    gauss->SetLineColor(kRed);
                    gauss->SetLineWidth(2);
                    gauss->Draw("same");
                    
                    TLatex *text = new TLatex();
                    text->SetNDC();
                    text->SetTextSize(0.04);
                    text->DrawLatex(0.18, 0.85, Form("Date: %s", date.c_str()));
                    text->DrawLatex(0.18, 0.80, Form("Module: %s (Ch %d)", ch.histogram_name.c_str(), ch_num));
                    text->DrawLatex(0.18, 0.75, Form("HV(mean)=%.1f V  VSet=%.0f V  (%s)", voltage_mean, hv_setpoint, voltage_quality.c_str()));
                    text->DrawLatex(0.18, 0.70, Form("Mean = (%.2f #pm %.2f) nA", mean, mean_err));
                    text->DrawLatex(0.18, 0.65, Form("#sigma = (%.2f #pm %.2f) nA", sigma, sigma_err));
                    text->DrawLatex(0.18, 0.60, Form("#sigma / Mean = %.2f #pm %.2f", ratio, ratio_err));
                } else {
                    std::cout << "Warning: Refined fit failed with status " << fit_status << std::endl;
                }
            } else {
                std::cout << "Warning: Initial fit failed with status " << fit_status << std::endl;
            }
            
            hist->GetXaxis()->SetTitle("Current (nA)");
            hist->GetYaxis()->SetTitle("Counts");
        } else {
            hist->GetXaxis()->SetTitle("Current (nA)");
            hist->GetYaxis()->SetTitle("Yield");
            
            TLatex *text = new TLatex();
            text->SetNDC();
            text->SetTextSize(0.04);
            text->DrawLatex(0.18, 0.85, Form("Date: %s", date.c_str()));
            text->DrawLatex(0.18, 0.80, Form("Module: %s (Ch %d)", ch.histogram_name.c_str(), ch_num));
            text->DrawLatex(0.18, 0.75, Form("HV: %.1f V (%s)", voltage_mean, voltage_quality.c_str()));
        }
        
        // Save output
        TString tstr_date(date);
        tstr_date.ReplaceAll("/", "_");
        tstr_date.ReplaceAll(",", "_");
        tstr_date.ReplaceAll(":", "_");
        tstr_date.ReplaceAll(" ", "_");
        
        std::string filename = Form("Xscan_%s_ch%d_%s.png", 
                                   ch.histogram_name.c_str(), ch_num, tstr_date.Data());
        std::string output_folder = "./";
        std::string file_out = output_folder + filename;
        c1->SaveAs(file_out.c_str());
        
        // Save JSON log with proper precision
        std::string log_filename = output_folder + Form("Xscan_%s_ch%d_%s.json",
                                                        ch.histogram_name.c_str(), ch_num, tstr_date.Data());
        
        std::ofstream log_file(log_filename.c_str());
        if (log_file.is_open()) {
            bool is_low = (ch.rate == "low");
            
            log_file << "{\n";
            log_file << "  \"detector\": \"" << ch.histogram_name << "\",\n";
            log_file << "  \"layer\": " << ch.layer << ",\n";
            log_file << "  \"module\": " << ch.moduleNumber << ",\n";
            log_file << "  \"date\": \"" << date << "\",\n";
            log_file << "  \"PS_channel\": " << ch_num << ",\n";
            log_file << "  \"rate\": \"" << ch.rate << "\",\n";

                        // Scan parameters
            log_file << "  \"scan_parameters\": {\n";
            log_file << "    \"x_step_mm\": " << xstep << ",\n";
            log_file << "    \"y_step_mm\": " << ystep << ",\n";
            log_file << "    \"scan_speed_mm_per_sec\": " << scanspeed << ",\n";
            log_file << "    \"standby_time_sec\": " << standbytime << ",\n";
            log_file << "    \"x_collimator_size_mm\": " << xcollimatorsize << ",\n";
            log_file << "    \"y_collimator_size_mm\": " << ycollimatorsize << "\n";
            log_file << "  },\n";
            
            // Add voltage statistics with proper precision
            log_file << std::fixed << std::setprecision(2);
            log_file << "  \"PS_voltage_ch_" << ch_num << "_mean\": " << voltage_mean << ",\n";
            log_file << "  \"PS_voltage_ch_" << ch_num << "_std\": " << voltage_std << ",\n";
            log_file << "  \"HV_setpoint_V\": " << (std::isnan(hv_setpoint) ? "null" : Form("%.0f", hv_setpoint)) << ",\n";
            log_file << "  \"HV_quality\": \"" << voltage_quality << "\",\n";
            log_file << "  \"HV_diff_V\": " << (std::isnan(hv_diff) ? "null" : Form("%.2f", hv_diff)) << ",\n";
            log_file << "  \"HV_tolerance_V\": " << (std::isnan(hv_tolerance) ? "null" : Form("%.2f", hv_tolerance)) << ",\n";

            log_file << "  \"xray\": {\n";
            log_file << "    \"tubes\": [\n";

bool first = true;
for (int t = 0; t < kMaxTubes; ++t) {
    bool haveAny = !std::isnan(xray_vset_kv[t]) || !std::isnan(xray_iset_ua[t]) || !std::isnan(xray_temp_mean_C[t]) || !xray_sn[t].empty();
    if (!haveAny) continue;

    if (!first) log_file << ",\n";
    first = false;

    log_file << "      {\n";
    log_file << "        \"tube\": " << (t+1) << ",\n";
    log_file << "        \"SN\": \"" << (xray_sn[t].empty() ? "NA" : xray_sn[t]) << "\",\n";

    if (std::isnan(xray_vset_kv[t])) log_file << "        \"VSet_kV\": null,\n";
    else log_file << "        \"VSet_kV\": " << std::fixed << std::setprecision(2) << xray_vset_kv[t] << ",\n";

    if (std::isnan(xray_iset_ua[t])) log_file << "        \"ISet_uA\": null,\n";
    else log_file << "        \"ISet_uA\": " << std::fixed << std::setprecision(2) << xray_iset_ua[t] << ",\n";

    if (std::isnan(xray_temp_mean_C[t])) log_file << "        \"TempMean_C\": null\n";
    else log_file << "        \"TempMean_C\": " << std::fixed << std::setprecision(2) << xray_temp_mean_C[t] << "\n";

    log_file << "      }";
}

log_file << "\n    ]\n";
log_file << "  },\n";

            log_file << "  \"data_points\": " << data_in_range.size() << ",\n";
            
            if (is_low) {
                log_file << std::fixed << std::setprecision(2);
                log_file << "  \"Current_fit_mean\": " << fit_mean << ",\n";
                log_file << "  \"Current_fit_sigma\": " << fit_sigma << "\n";
            } else {
                log_file << "  \"Current_fit_mean\": null,\n";
                log_file << "  \"Current_fit_sigma\": null\n";
            }
            
            log_file << "}\n";
            log_file.close();
        }
        
        delete hist;
        delete c1;
    }
    
    // Cleanup
    for (auto& p : profiles) {
        delete p.second;
    }
    
    return 0;
}