/***********************************************************
* Author: Andrew Belcher
* Desc: header file for correlation/differential power 
* analysis trace aquisition mod for Dream Source Labs DSView
*
* Modify start offset to reflect where to trim the trace output from
* and the size of the trace to save to csv.
*
* TODO: Set these defines as parameters in the UI
************************************************************/
#define CPA_SAMPLE_COUNT_SIZE 50000
#define CPA_SAMPLE_COUNT_START 25000
#define CPA_SAMPLE_COUNT_END (CPA_SAMPLE_COUNT_START + CPA_SAMPLE_COUNT_SIZE)