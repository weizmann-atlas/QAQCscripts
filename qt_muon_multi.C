#if !defined(__CINT__) || defined(__MAKECINT__)

 #include <iostream>
 #include <fstream>
 #include <regex>
 #include <string>
 #include <vector>
 #include <cmath>
 #include <map>
 #include <iomanip>
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
        std::regex pattern(R"(# (T11|EIL4|T#B|T#F|M1|M2|M3|Bs|Fs-L1|Fs-L2|Fs-L3|T11Fs)([A-Z]?) # layer # (\d+) # module # (\d+) # PS ch # (\d+) # rate # (\w+))");
        std::smatch matches;
        
        if (std::regex_search(line, matches, pattern)) {
            ChannelInfo ch;
            ch.moduleType = matches[1];
            ch.moduleSuffix = matches[2];
            ch.layer = matches[3];
            ch.moduleNumber = matches[4];
            ch.psChannel = std::stoi(matches[5].str());
            ch.rate = matches[6];
            ch.histogram_name = ch.moduleType + ch.moduleSuffix + "-L" + ch.layer + "-Module-" + ch.moduleNumber;
            
            channels[ch.psChannel] = ch;
            pattern_found = true;
            
            std::cout << "Found channel " << ch.psChannel << ": " << ch.histogram_name 
                      << " (rate: " << ch.rate << ")" << std::endl;
        }
        
        // Parse HV values
        if (line.find("HV supply voltage:") != std::string::npos) {
            std::string hv_line = line.substr(line.find(":") + 2);
            std::istringstream iss(hv_line);
            double value;
            while (iss >> value) {
                hv_values.push_back(value);
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
    
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        double x, y, Np, i1, i2, i3, i4, v1, v2, v3, v4;
        
        if (!(iss >> x >> y >> Np >> i1 >> i2 >> i3 >> i4 >> v1 >> v2 >> v3 >> v4)) {
            continue;
        }
        
        std::vector<double> currents = {i1, i2, i3, i4};
        std::vector<double> volts = {v1, v2, v3, v4};
        
        for (auto& ch_pair : channels) {
            int ch_num = ch_pair.first;
            int col_idx = channel_to_column[ch_num];
            
            profiles[ch_num]->Fill(x, y, currents[col_idx]);
            current_data[ch_num].push_back(currents[col_idx]);
            x_coords[ch_num].push_back(x);
            voltage_data[ch_num].push_back(volts[col_idx]);  // Store voltage values
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
        
        // Better binning: use Sturges' rule or fixed number
        int nbins = std::max(50, std::min(200, static_cast<int>(1 + 3.322 * log(data_in_range.size()))));
        
        // TH1D *hist = new TH1D(Form("I_ch%d", ch_num), "Current", nbins, minI, maxI);
        TH1D *hist = new TH1D(Form("I_ch%d", ch_num), "Current", 200, 600, 900);        
        for (double val : data_in_range) {
            hist->Fill(val);
        }
        
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
                    text->DrawLatex(0.18, 0.75, Form("HV: %.1f V", voltage_mean));
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
            text->DrawLatex(0.18, 0.75, Form("HV: %.1f V", voltage_mean));
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
            log_file << "    \"x_step\": " << xstep << ",\n";
            log_file << "    \"y_step\": " << ystep << ",\n";
            log_file << "    \"scan_speed\": " << scanspeed << ",\n";
            log_file << "    \"standby_time\": " << standbytime << ",\n";
            log_file << "    \"x_collimator_size\": " << xcollimatorsize << ",\n";
            log_file << "    \"y_collimator_size\": " << ycollimatorsize << "\n";
            log_file << "  },\n";
            
            // Add voltage statistics with proper precision
            log_file << std::fixed << std::setprecision(2);
            log_file << "  \"PS_voltage_ch_" << ch_num << "_mean\": " << voltage_mean << ",\n";
            log_file << "  \"PS_voltage_ch_" << ch_num << "_std\": " << voltage_std << ",\n";
            
            log_file << "  \"data_points\": " << data_in_range.size() << ",\n";
            
            if (is_low) {
                log_file << std::fixed << std::setprecision(2);
                log_file << "  \"fit_mean\": " << fit_mean << ",\n";
                log_file << "  \"fit_sigma\": " << fit_sigma << "\n";
            } else {
                log_file << "  \"fit_mean\": null,\n";
                log_file << "  \"fit_sigma\": null\n";
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