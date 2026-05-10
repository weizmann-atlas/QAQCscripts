#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <map>
#include <vector>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <set>

// ROOT includes
#include <TCanvas.h>
#include <TGraph.h>
#include <TH2D.h>
#include <TAxis.h>
#include <TLegend.h>
#include <TString.h>
#include <TPad.h>
#include <TLatex.h>

// Structure to store header information
struct HeaderInfo {
    std::map<std::string, std::string> params;
    std::vector<int> channels;
    long start_timestamp;
    int header_index;
};

// Function to extract date from a data line
std::string extract_date(const std::string& line) {
    std::smatch match;
    std::regex date_regex(R"(\[(\d{4}-\d{2}-\d{2})T\d{2}:\d{2}:\d{2}\]:)");
    if (std::regex_search(line, match, date_regex)) {
        return match[1];
    }
    return "Unknown_Date";
}

// Function to parse timestamp from line
long parse_timestamp(const std::string& line) {
    std::smatch match;
    std::regex timestamp_regex(R"(\[(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})\]:)");
    if (std::regex_search(line, match, timestamp_regex)) {
        std::string timestamp_str = match[1].str();
        std::istringstream ss(timestamp_str);
        std::tm tm = {};
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (!ss.fail()) {
            return std::mktime(&tm);
        }
    }
    return -1;
}

// Function to parse header line correctly
std::map<std::string, std::string> parse_generic_header_line(const std::string& line) {
    std::map<std::string, std::string> params;
    
    // Remove leading '#' and split by '#'
    std::string clean_line = line;
    if (!clean_line.empty() && clean_line[0] == '#') {
        clean_line = clean_line.substr(1);
    }
    
    std::vector<std::string> tokens;
    std::stringstream ss(clean_line);
    std::string token;
    
    while (std::getline(ss, token, '#')) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    // The first non-empty token is the detector type (e.g., "EIL4B")
    if (!tokens.empty()) {
        std::string detector = tokens[0];
        params["Detector"] = detector;
        
        // Split detector into type and suffix if it ends with a letter
        if (detector.length() > 1 && std::isalpha(detector.back())) {
            params["ModuleType"] = detector.substr(0, detector.length()-1);
            params["ModuleSuffix"] = std::string(1, detector.back());
        } else {
            params["ModuleType"] = detector;
            params["ModuleSuffix"] = "";
        }
    }
    
    // Process remaining tokens as key-value pairs
    for (size_t i = 1; i + 1 < tokens.size(); i += 2) {
        std::string key = tokens[i];
        std::string value = tokens[i + 1];
        params[key] = value;
    }
    
    return params;
}

// Helper to sanitize string for filenames
std::string sanitize_filename_part(std::string s) {
    std::replace(s.begin(), s.end(), '/', '_');
    std::replace(s.begin(), s.end(), ' ', '_');
    std::replace(s.begin(), s.end(), ':', '_');
    return s;
}

// Function to parse channel list
std::vector<int> parse_channel_list(const std::string& ch_str) {
    std::vector<int> channels;
    if (ch_str.empty() || ch_str == "N/A") {
        return channels;
    }
    
    std::stringstream ss(ch_str);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        
        try {
            int ch = std::stoi(token);
            if (ch >= 0 && ch <= 15) {
                channels.push_back(ch);
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Invalid channel number: '" << token << "'" << std::endl;
        }
    }
    
    return channels;
}

// Function to create plot for a specific header configuration
void create_plot(const HeaderInfo& header, 
                 const std::map<int, std::map<double, double>>& vmon_data,
                 const std::map<int, std::map<double, double>>& imon_data,
                 const std::string& output_dir,
                 const std::string& date_str) {
    
    auto& params = header.params;
    
    // Extract header parameters
    std::string module_type = params.count("ModuleType") ? sanitize_filename_part(params.at("ModuleType")) : "Unknown";
    std::string module_suffix = params.count("ModuleSuffix") ? sanitize_filename_part(params.at("ModuleSuffix")) : "";
    std::string layer = params.count("layer") ? sanitize_filename_part(params.at("layer")) : "N_A";
    std::string module = params.count("module") ? sanitize_filename_part(params.at("module")) : "N_A";
    std::string stage = params.count("Stage") ? sanitize_filename_part(params.at("Stage")) : "N_A";
    std::string gas = params.count("Gas") ? sanitize_filename_part(params.at("Gas")) : "N_A";
    std::string qc = params.count("QC") ? sanitize_filename_part(params.at("QC")) : "N_A";
    
    // Get all channels that have data
    std::set<int> all_channels;
    for (const auto& pair : vmon_data) {
        all_channels.insert(pair.first);
    }
    for (const auto& pair : imon_data) {
        all_channels.insert(pair.first);
    }
    
    // Process each channel that has data
    for (int channel : all_channels) {
        std::cout << "  Creating plot for channel " << channel << std::endl;
        
        const auto& vmon = vmon_data.count(channel) ? vmon_data.at(channel) : std::map<double, double>();
        const auto& imon = imon_data.count(channel) ? imon_data.at(channel) : std::map<double, double>();
        
        if (vmon.empty() && imon.empty()) continue;
        
        // Prepare arrays for TGraph
        int n_vmon = vmon.size();
        double* x_vmon = nullptr;
        double* y_vmon = nullptr;
        if (n_vmon > 0) {
            x_vmon = new double[n_vmon];
            y_vmon = new double[n_vmon];
            int i = 0;
            for (const auto& pair : vmon) {
                x_vmon[i] = pair.first;
                y_vmon[i] = pair.second;
                i++;
            }
        }
        
        int n_imon = imon.size();
        double* x_imon = nullptr;
        double* y_imon = nullptr;
        if (n_imon > 0) {
            x_imon = new double[n_imon];
            y_imon = new double[n_imon];
            int i = 0;
            for (const auto& pair : imon) {
                x_imon[i] = pair.first;
                y_imon[i] = pair.second;
                i++;
            }
        }
        
        // Determine COMMON time range using the FIRST timestamp across both datasets
        double min_time = 0, max_time = 300;
        if (n_vmon > 0 && n_imon > 0) {
            min_time = std::min(x_vmon[0], x_imon[0]);
            max_time = std::max(x_vmon[n_vmon-1], x_imon[n_imon-1]);
        } else if (n_vmon > 0) {
            min_time = x_vmon[0];
            max_time = x_vmon[n_vmon-1];
        } else if (n_imon > 0) {
            min_time = x_imon[0];
            max_time = x_imon[n_imon-1];
        }
        
        // Add padding
        double range = max_time - min_time;
        if (range > 0) {
            min_time -= range * 0.05;
            max_time += range * 0.05;
        }
        
        // Determine VMon Y-axis range with auto-scaling
        double vmon_min = 0, vmon_max = 4500;
        if (n_vmon > 0) {
            vmon_min = y_vmon[0];
            vmon_max = y_vmon[0];
            for (int i = 0; i < n_vmon; i++) {
                if (y_vmon[i] < vmon_min) vmon_min = y_vmon[i];
                if (y_vmon[i] > vmon_max) vmon_max = y_vmon[i];
            }
            // Add 10% padding
            double vmon_range = vmon_max - vmon_min;
            if (vmon_range < 1.0) vmon_range = 1.0;
            vmon_min -= vmon_range * 0.1;
            vmon_max += vmon_range * 0.1;
            if (vmon_min < 0) vmon_min = 0;
        }
        
        // Determine IMon Y-axis range with auto-scaling
        double imon_min = 0, imon_max = 0.15;
        if (n_imon > 0) {
            imon_min = y_imon[0];
            imon_max = y_imon[0];
            for (int i = 0; i < n_imon; i++) {
                if (y_imon[i] < imon_min) imon_min = y_imon[i];
                if (y_imon[i] > imon_max) imon_max = y_imon[i];
            }
            // Add 10% padding
            double imon_range = imon_max - imon_min;
            if (imon_range < 0.001) imon_range = 0.001;
            imon_min -= imon_range * 0.1;
            imon_max += imon_range * 0.1;
            if (imon_min < 0) imon_min = 0;
        }
        
        // Create canvas
        std::string canvas_title = Form("VMon and IMon (PS Ch %d) - %s ", 
                                        channel, date_str.c_str(), header.header_index);
        std::string canvas_name = Form("c_h%d_ch%d", header.header_index, channel);
        TCanvas* c = new TCanvas(canvas_name.c_str(), canvas_title.c_str(), 1000, 700);
        
        // Create first pad for VMon
        TPad *pad1 = new TPad(Form("pad1_h%d_ch%d", header.header_index, channel), "", 0, 0, 1, 1);
        pad1->SetFillStyle(4000);
        pad1->SetFrameFillStyle(0);
        pad1->Draw();
        pad1->cd();
        
        TGraph* graph_vmon = nullptr;
        if (n_vmon > 0) {
            graph_vmon = new TGraph(n_vmon, x_vmon, y_vmon);
            graph_vmon->SetTitle("");
            graph_vmon->SetMarkerStyle(20);
            graph_vmon->SetMarkerColor(kRed);
            graph_vmon->SetLineColor(kRed);
            graph_vmon->GetYaxis()->SetAxisColor(kRed);
            graph_vmon->GetYaxis()->SetLabelColor(kRed);
            graph_vmon->GetYaxis()->SetAxisColor(kRed);
            graph_vmon->GetXaxis()->SetTitle("Time [s]");
            graph_vmon->GetYaxis()->SetTitle("VMon [V]");
            graph_vmon->GetYaxis()->SetTitleColor(kRed);
            graph_vmon->GetXaxis()->SetLimits(min_time, max_time);
            graph_vmon->GetYaxis()->SetRangeUser(vmon_min, vmon_max);
            graph_vmon->Draw("APL");
        } else {
            TH2D* h_frame = new TH2D(Form("h_vmon_h%d_ch%d", header.header_index, channel), 
                                     "", 100, min_time, max_time, 100, vmon_min, vmon_max);
            h_frame->GetXaxis()->SetTitle("Time [s]");
            h_frame->GetYaxis()->SetTitle("VMon [V]");
            h_frame->GetYaxis()->SetTitleOffset(1.3);
            h_frame->GetYaxis()->SetAxisColor(kRed);
            h_frame->GetYaxis()->SetTitleColor(kRed);
            h_frame->GetYaxis()->SetLabelColor(kRed);
            h_frame->Draw();
        }
        
        // Create second pad for IMon
        c->cd();
        TPad *pad2 = new TPad(Form("pad2_h%d_ch%d", header.header_index, channel), "", 0, 0, 1, 1);
        pad2->SetFillStyle(4000);
        pad2->SetFrameFillStyle(0);
        pad2->Draw();
        pad2->cd();
        
        TGraph* graph_imon = nullptr;
        if (n_imon > 0) {
            graph_imon = new TGraph(n_imon, x_imon, y_imon);
            graph_imon->SetTitle("");
            graph_imon->SetMarkerStyle(21);
            graph_imon->SetMarkerColor(kBlue);
            graph_imon->SetLineColor(kBlue);
            graph_imon->GetYaxis()->SetTitle("IMon [uA]");
            graph_imon->GetYaxis()->SetTitleOffset(1.3);
            graph_imon->GetYaxis()->SetTitleColor(kBlue);
            graph_imon->GetYaxis()->SetLabelOffset(0.015);
            graph_imon->GetYaxis()->SetAxisColor(kBlue);
            graph_imon->GetYaxis()->SetLabelColor(kBlue);
            graph_imon->GetXaxis()->SetLimits(min_time, max_time);
            graph_imon->GetYaxis()->SetRangeUser(imon_min, imon_max);
            graph_imon->Draw("APLY+");
        } else {
            TH2D* h_frame = new TH2D(Form("h_imon_h%d_ch%d", header.header_index, channel),
                                     "", 100, min_time, max_time, 100, imon_min, imon_max);
            h_frame->GetYaxis()->SetTitle("IMon [uA]");
            h_frame->GetYaxis()->SetTitleOffset(1.3);
            h_frame->GetYaxis()->SetTitleColor(kBlue);
            h_frame->GetYaxis()->SetLabelOffset(0.015);
            h_frame->GetYaxis()->SetAxisColor(kBlue);
            h_frame->GetYaxis()->SetLabelColor(kBlue);
            h_frame->Draw("AXIS SAME");
        }
        
        // Add title
        pad1->cd();
        TLatex *title = new TLatex();
        title->SetTextAlign(22);
        title->SetTextFont(42);
        title->SetTextSize(0.035);
        title->DrawLatexNDC(0.5, 0.95, canvas_title.c_str());
        
        // Create legend
        TLegend* leg = new TLegend(0.2, 0.58, 0.5, 0.88);
        if (graph_vmon) leg->AddEntry(graph_vmon, "VMon", "lp");
        if (graph_imon) leg->AddEntry(graph_imon, "IMon", "lp");
        
        // Add header info to legend
        if (!module_type.empty() && module_type != "Unknown")
            leg->AddEntry((TObject*)0, Form("Module: %s%s", module_type.c_str(), module_suffix.c_str()), "");
        if (layer != "N_A")
            leg->AddEntry((TObject*)0, Form("Layer: %s", layer.c_str()), "");
        if (module != "N_A")
            leg->AddEntry((TObject*)0, Form("Module #: %s", module.c_str()), "");
        if (stage != "N_A")
            leg->AddEntry((TObject*)0, Form("Stage: %s", stage.c_str()), "");
        if (gas != "N_A")
            leg->AddEntry((TObject*)0, Form("Gas: %s", gas.c_str()), "");
        if (qc != "N_A")
            leg->AddEntry((TObject*)0, Form("QC: %s", qc.c_str()), "");
        
        leg->SetBorderSize(0);
        leg->SetFillStyle(0);
        leg->Draw();
        
        c->Update();
        c->Modified();
        
        // Save plot
        std::string output_filename = output_dir + "/vmon_imon_" +
                                     module_type + module_suffix + 
                                     "_L" + layer + "_M" + module +
                                     "_CH" + std::to_string(channel) +
                                     "_H" + std::to_string(header.header_index) +
                                     "_" + date_str + ".png";
        c->SaveAs(output_filename.c_str());
        std::cout << "Plot saved: " << output_filename << std::endl;
        
        // Cleanup
        if (x_vmon) delete[] x_vmon;
        if (y_vmon) delete[] y_vmon;
        if (x_imon) delete[] x_imon;
        if (y_imon) delete[] y_imon;
        delete c;
    }
}

int Read_HV_Log_multi(std::string fname) {
    std::ifstream infile(fname);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << fname << std::endl;
        return 1;
    }

    std::string line;
    std::vector<std::string> all_lines;
    
    while (std::getline(infile, line)) {
        all_lines.push_back(line);
    }
    infile.close();
    
    std::vector<HeaderInfo> headers;
    std::regex data_line_regex(R"(\[(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})\]: \[.*\] bd \[(\d+)\] ch \[(\d+)\] par \[(IMon|IMonH|VMon)\] val \[([\d.eE+-]+)\])");
    
    int header_counter = 0;
    std::string current_date = "Unknown_Date";
    long first_timestamp = -1;
    
    // Phase 1: Identify all headers and their timestamps
    for (size_t idx = 0; idx < all_lines.size(); idx++) {
        const std::string& current_line = all_lines[idx];
        if (current_line.empty()) continue;
        
        if (current_line[0] == '#') {
            HeaderInfo header;
            header.params = parse_generic_header_line(current_line);
            header.header_index = header_counter++;
            
            std::cout << "\nHeader " << header.header_index << " parsed:" << std::endl;
            std::cout << "  Detector: " << header.params["Detector"] << std::endl;
            std::cout << "  Layer: " << header.params["layer"] << std::endl;
            std::cout << "  Module: " << header.params["module"] << std::endl;
            std::cout << "  PS board: " << header.params["PS board"] << std::endl;
            std::cout << "  PS ch: " << header.params["PS ch"] << std::endl;
            std::cout << "  Stage: " << header.params["Stage"] << std::endl;
            std::cout << "  QC: " << header.params["QC"] << std::endl;
            std::cout << "  Gas: " << header.params["Gas"] << std::endl;
            
            // Parse channels
            if (header.params.count("PS ch")) {
                header.channels = parse_channel_list(header.params["PS ch"]);
                std::cout << "  Channels: ";
                for (int ch : header.channels) {
                    std::cout << ch << " ";
                }
                std::cout << std::endl;
            }
            
            // Find timestamp of next data line
            header.start_timestamp = -1;
            for (size_t j = idx + 1; j < all_lines.size(); j++) {
                long ts = parse_timestamp(all_lines[j]);
                if (ts != -1) {
                    header.start_timestamp = ts;
                    if (current_date == "Unknown_Date") {
                        current_date = extract_date(all_lines[j]);
                    }
                    if (first_timestamp == -1) {
                        first_timestamp = ts;
                    }
                    break;
                }
            }
            
            if (header.start_timestamp != -1) {
                headers.push_back(header);
            }
        }
    }
    
    if (headers.empty()) {
        std::cerr << "Error: No valid headers found" << std::endl;
        return 1;
    }
    
    // Phase 2: Parse data and assign to headers
    // For each header, collect data for its specific channel(s)
    std::map<int, std::map<int, std::map<double, double>>> vmon_by_header;
    std::map<int, std::map<int, std::map<double, double>>> imon_by_header;
    
    for (const std::string& current_line : all_lines) {
        if (current_line.empty() || current_line[0] == '#') continue;
        
        std::smatch match;
        if (std::regex_search(current_line, match, data_line_regex)) {
            long timestamp = parse_timestamp(current_line);
            if (timestamp == -1) continue;
            
            int channel = std::stoi(match[3].str());
            std::string parameter = match[4].str();
            double value = std::stod(match[5].str());
            double relative_time = static_cast<double>(timestamp - first_timestamp);
            
            // Check ALL headers to see if this channel belongs to any of them
            for (const HeaderInfo& header : headers) {
                // Check if this channel is in this header's channel list
                bool channel_matches = false;
                if (header.channels.empty()) {
                    // If no channels specified, accept all
                    channel_matches = true;
                } else {
                    // Check if channel is in the list
                    if (std::find(header.channels.begin(), header.channels.end(), channel) 
                        != header.channels.end()) {
                        channel_matches = true;
                    }
                }
                
                if (channel_matches) {
                    // Store data for this header
                    if (parameter == "VMon") {
                        vmon_by_header[header.header_index][channel][relative_time] = value;
                    } else if (parameter == "IMon" || parameter == "IMonH") {
                        imon_by_header[header.header_index][channel][relative_time] = value;
                    }
                }
            }
        }
    }
    
    // Phase 3: Generate plots
    std::string output_dir = ".";
    size_t last_slash = fname.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        output_dir = fname.substr(0, last_slash);
    }
    
    for (const HeaderInfo& header : headers) {
        std::cout << "\nProcessing header " << header.header_index << "..." << std::endl;
        
        std::set<int> channels_with_data;
        for (const auto& ch_pair : vmon_by_header[header.header_index]) {
            channels_with_data.insert(ch_pair.first);
        }
        for (const auto& ch_pair : imon_by_header[header.header_index]) {
            channels_with_data.insert(ch_pair.first);
        }
        
        std::cout << "  Channels with data: ";
        for (int ch : channels_with_data) {
            std::cout << ch << " ";
        }
        std::cout << std::endl;
        
        if (channels_with_data.empty()) {
            std::cout << "  No data found, skipping." << std::endl;
            continue;
        }
        
        create_plot(header, 
                   vmon_by_header[header.header_index],
                   imon_by_header[header.header_index],
                   output_dir,
                   current_date);
    }
    
    std::cout << "\nProcessing completed for " << headers.size() << " header(s)." << std::endl;
    return 0;
}

int main() {
    std::string filename = "HV_QC1_2025-10-28_13-56.log";
    Read_HV_Log_multi(filename);
    return 0;
}