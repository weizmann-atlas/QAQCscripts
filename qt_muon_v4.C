
#if !defined(__CINT__) || defined(__MAKECINT__)

 #include <iostream>
 #include <fstream>
 #include <regex>
 #include <string>
 #include <vector>
 #include <cmath>
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
 #include <limits> // For numeric limits

#endif

Int_t qt_muon_v4(std::string fname = "qscanner_out.2024-10-30_11-38.dat")
{
std::ifstream infile(fname);
std::string line, type, date, histogram_name;

double xmin, xmax, ymin, ymax;
int xnbins, ynbins;
double Volt;


std::string avg_hv;
std::string module_type;
std::string rate;
bool pattern_found = false;

for (int i = 0; i < 28; ++i) { // Assuming 28 header lines
    std::getline(infile, line);
    std::regex date_regex(R"((\w+, \d{1,2} \w+ \d{4} \d{2}:\d{2}:\d{2}))");
    std::smatch match;

    //std::regex pattern("# T11F # (\\d+), gap # (\\d+)"); // # T11B # 6, gap # 1
    //std::regex pattern("# (T[1-100][A-Z]) # (\\d+), gap # (\\d+)");
    //std::regex pattern(R"(# (T11|EIL4|T#B|T#F|M1|M2|M3|Bs|Fs-L1|Fs-L2|Fs-L3)([A-Z]) # (\d+), gap # (\d+))");
    std::regex pattern(R"(# (T11|EIL4|T#B|T#F|M1|M2|M3|Bs|Fs-L1|Fs-L2|Fs-L3)([A-Z]) # layer # (\d+) # module # (\d+) # PS ch # (\d+) # rate # (\w+))");
    std::smatch matches; 

    if (std::regex_search(line, match, date_regex)) {
            date = match[1];
            std::cout << "Date: " << date << std::endl;
    }

    if (std::regex_search(line, matches, pattern)) {
        std::string moduleType = matches[1];
        std::string moduleSuffix = matches[2];
        std::string layer = matches[3];
        std::string moduleNumber = matches[4];
        std::string psChannel = matches[5];
        rate = matches[6];
        pattern_found = true;

        std::cout << "Module Type: " << moduleType << std::endl;
        std::cout << "Module Suffix: " << moduleSuffix << std::endl;
        std::cout << "Layer: " << layer << std::endl;
        std::cout << "Module Number: " << moduleNumber << std::endl;
        std::cout << "PS Channel: " << psChannel << std::endl;
        std::cout << "Rate: " << rate << std::endl;
        histogram_name = moduleType + moduleSuffix + "-L" + layer + "-Module-" + moduleNumber; 
        std::cout << "Test histogram name: " << histogram_name << std::endl; 
    // } else if (i == 27) { // Check if it's the last header line and no match was found
    //     //std::cout << "No match found in line: " << line << std::endl; // Print the line for debugging
    }

    if (line.find("HV supply voltage:") != std::string::npos) {
        // Extract HV values
        std::string hv_line = line.substr(line.find(":") + 2);
        std::istringstream iss(hv_line);
        std::vector<double> hv_values;
        double value;
            while (iss >> value) {
                hv_values.push_back(value);
            }
            
            // a vector of HV values:
            for (double hv : hv_values) {
                std::cout << "HV value: " << hv << std::endl;
            }
    }

    // Extract parameters using string manipulation or regular expressions
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
        xnbins = std::stod(value);
    }
    if (line.find("NBINSY") != std::string::npos) {
        std::istringstream iss(line);
        std::string keyword, value;
        iss >> keyword >> value;
        ynbins = std::stod(value);
    }   
    // if (line.find("# T1") != std::string::npos) {
    // }
}
if (!pattern_found) {
    std::cout << "Warning: No matching pattern found in the header. Expected format: '# EIL4B # layer # 3 # module # 10 # PS ch # 2 # rate # high'" << std::endl;
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

// Book the histogram
    gStyle->SetPadRightMargin(0.15);
    gStyle->SetPadLeftMargin(0.15);

    TProfile2D *IvsXY3 = new TProfile2D("IvsXY3", "x-ray scan I3", xnbins, xmin, xmax, ynbins, ymin, ymax, 0, 0);

// Variables to track min and max values for I3 and I4
double minI3 = std::numeric_limits<double>::max();
double maxI3 = std::numeric_limits<double>::lowest();
double minI4 = std::numeric_limits<double>::max();
double maxI4 = std::numeric_limits<double>::lowest();

// Read data lines
while (std::getline(infile, line)) {
    std::istringstream iss(line);
    double x, y, Np, t, i1, i2, i3, i4, v1, v2, v3, v4;
    iss >> x >> y >> Np  >> i1 >> i2 >> i3 >> i4 >> v1 >> v2 >> v3 >> v4;

    IvsXY3->Fill(x, y, i3);

    // Update min and max values for I3 and I4
    if (i3 < minI3) minI3 = i3;
    if (i3 > maxI3) maxI3 = i3;
    if (i4 < minI4) minI4 = i4;
    if (i4 > maxI4) maxI4 = i4;

    Volt = v3;
}

// Determine the range and number of bins for I3 and I4
int nbinsI3 = static_cast<int>(std::sqrt(maxI3 - minI3)); // Rule of thumb: sqrt(range)
int nbinsI4 = static_cast<int>(std::sqrt(maxI4 - minI4));

// Ensure a minimum number of bins
if (nbinsI3 <= 0) nbinsI3 = 50; // Set a minimum number of bins
if (nbinsI4 <= 0) nbinsI4 = 50; // Set a minimum number of bins
// Book the histograms with dynamic binning
TH1D *I3 = new TH1D("I3", "Current", nbinsI3, minI3, maxI3);
TH1D *I4 = new TH1D("I4", "Current", nbinsI4, minI4, maxI4);

// Rewind the file and read data again to fill the histograms
infile.clear();
infile.seekg(0, std::ios::beg);

while (std::getline(infile, line)) {
    std::istringstream iss(line);
    double x, y, Np, t, i1, i2, i3, i4, v1, v2, v3, v4;
    iss >> x >> y >> Np  >> i1 >> i2 >> i3 >> i4 >> v1 >> v2 >> v3 >> v4;
    I3->Fill(i3);
    I4->Fill(i4);
}

// Create a canvas with 2x2 pads
TCanvas *c1 = new TCanvas("c1", "Profiles", 1000, 800);
gStyle->SetOptStat(11);
c1->Divide(1, 2);
c1->SetFillColor(0);
c1->SetBorderMode(0);
c1->SetBorderSize(4);
c1->SetFrameBorderMode(0);
c1->SetFrameBorderMode(0);
// Draw the histograms in each pad
c1->cd(1);

IvsXY3->Draw("COLZ");
IvsXY3->GetXaxis()->SetTitle("X coordinate (mm)");
IvsXY3->GetYaxis()->SetTitle("Y coordinate (mm)");

// Draw and save the histogram
c1->cd(2);
// Add grid lines to the canvas
gPad->SetGridx();
gPad->SetGridy();
I3->SetStats(0);
//I3->Rebin(4);
I3->Draw("hist");
I3->SetFillColor(kYellow);

if (rate == "low") { // Check if rate is "low"
    // Find the bin with the highest count
    int max_bin = I3->GetMaximumBin();
    double max_x = I3->GetBinCenter(max_bin);
    // Set fit range (adjust as needed)
    double fit_low = max_x - 2.5 * I3->GetRMS();
    double fit_high = max_x + 2.5 * I3->GetRMS();

    // Create a Gaussian function
    TF1 *gauss = new TF1("gauss", "gaus", fit_low, fit_high);
    gauss->SetParameters(I3->GetMaximum(), max_x, I3->GetRMS());
    I3->Fit(gauss, "R");
    // Get the fit parameters and errors
    double mean = gauss->GetParameter(1);
    double sigma = gauss->GetParameter(2);
    double mean_err = gauss->GetParError(1);
    double sigma_err = gauss->GetParError(2);
    double ratio = sigma / mean;
    double ratio_err = ratio * sqrt(pow(sigma_err / sigma, 2) + pow(mean_err / mean, 2));

    // Draw the fit
    gauss->Draw("same");
    I3->GetXaxis()->SetTitle("Current (nA)");
    I3->GetYaxis()->SetTitle("Yeild");

    // Add a text box with fit parameters
    TLatex *text = new TLatex();
    text->SetNDC();
    text->SetTextSize(0.04);
    //text->DrawLatex(0.25, 0.90, Form("Xray Scan:: %s", type.c_str()));
    text->DrawLatex(0.18, 0.85, Form("Date: %s", date.c_str()));
    text->DrawLatex(0.18, 0.80, Form("Module:  %s", histogram_name.c_str()));
    text->DrawLatex(0.18, 0.75, Form("HV: %.1f V", Volt));
    text->DrawLatex(0.18, 0.70, Form("Mean = (%.2f #pm %.2f) nA", mean, mean_err));
    text->DrawLatex(0.18, 0.65, (Form("#sigma = (%.2f #pm %.2f) nA", sigma, sigma_err)));
    text->DrawLatex(0.18, 0.60, (Form("#sigma / Mean = %.2f #pm %.2f", ratio, ratio_err)));
} else {
    // If rate is not "low", just draw the histogram without the fit
    I3->GetXaxis()->SetTitle("Current (nA)");
    I3->GetYaxis()->SetTitle("Yeild");
    TLatex *text = new TLatex();
    text->SetNDC();
    text->SetTextSize(0.04);
    text->DrawLatex(0.18, 0.85, Form("Date: %s", date.c_str()));
    text->DrawLatex(0.18, 0.80, Form("Module:  %s", histogram_name.c_str()));
    text->DrawLatex(0.18, 0.75, Form("HV: %.1f V", Volt));
}

//c1->SaveAs("Xscan.png");
TString tstr_date(date);
tstr_date.ReplaceAll("/", "_");
tstr_date.ReplaceAll(",", "_");
tstr_date.ReplaceAll(":", "_");
tstr_date.ReplaceAll(" ", "_");
std::string filename_replaced = Form("Xscan_%s_%s.png", histogram_name.c_str(), tstr_date.Data());
// Create the output filename
    std::string output_folder = "/eos/project/w/wis-tgc/public/output_plots/"; // Replace with your desired output folder
    std::string file_out = output_folder + filename_replaced;
    c1->SaveAs(file_out.c_str());
    // Save the canvas to the "output" folder
    //c1->SaveAs(filename_replaced.c_str()); 
  return 0;
}
