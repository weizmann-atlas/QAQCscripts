EIL4 Chamber Production: Automating QA/QC
The production of EIL4 chambers is founded upon well-established Thin Gap Chamber (TGC) expertise. 
To date, a comprehensive Quality Assurance/Quality Control (QA/QC) procedure is manually implemented to ensure the final quality of each detector.
The automation of data analysis and the generation of high-quality plots for routine TGC chamber test results are developed



The Header Builder - loggui_v2.html generates headers for X ray and HV scans; should be used as a public URL and everything could be filled in the browser, 
Right-click on file → Open with → Chrome / Firefox without requiring any additional files

To run Xray scan precessing: root -l qt_muon_multi.C'("Xray_scans/qscanner_out.2025-01-28_03-32.dat")'

To run HV scan: root -l Read_HV_Log_multi.C'("HV_QC4_2026-01-13_17-53.log")'
Creates a struct to store each header's parameters, channels, timestamp, and unique index.
1: Scans the entire file to identify all headers and determines when each becomes active (based on the timestamp of the next data line)
2: Processes all data lines and assigns them to the correct header based on timestamps
3: Generates separate plots for each header configuration
