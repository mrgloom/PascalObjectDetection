#include "PrecisionRecall.h"

using namespace std;

static void computePrecisionRecallForThreshold(const vector<float> &gt, const vector<float>& preds, 
	                               float threshold, int nGroundTruthDetections, float& precision, float& recall)
{
	// Compute tp, fp, fn, tn
	int truePos = 0, trueNeg = 0, falsePos = 0, falseNeg = 0;
	for(int i = 0; i < preds.size(); i++) {
		if(preds[i] > threshold) {
			if(gt[i] > 0) truePos++;
			else falsePos++;
		} else if(preds[i] <= threshold) {
			if(gt[i] < 0) trueNeg++;
			else falseNeg++;			
		}
	}

	if(truePos + falsePos == 0) precision = 1.0;
	else precision = float(truePos) / (truePos + falsePos);

	int nGt = (nGroundTruthDetections >= 0)? nGroundTruthDetections:(truePos + falseNeg);

	if(truePos + falseNeg == 0) recall = 1.0;
	else recall = float(truePos) / nGt;
}

bool sortByRecall(const PrecisionRecallPoint& a, const PrecisionRecallPoint& b)
{
	return a.recall < b.recall;
}

PrecisionRecall::PrecisionRecall(const std::vector<float> &gt, const std::vector<float>& preds, int nGroundTruthDetections)
{
	std::set<float> thresholds;
	for (int i = 0; i < preds.size(); ++i) {
		thresholds.insert(preds[i]);
	}

	_data.resize(0);

	for(std::set<float>::iterator th = thresholds.begin(); th != thresholds.end(); th++) {
		PrecisionRecallPoint pr;
		computePrecisionRecallForThreshold(gt, preds, *th, nGroundTruthDetections, pr.precision, pr.recall); 
		pr.threshold = *th;

		_data.push_back(pr);
	}

	std::sort(_data.begin(), _data.end(), sortByRecall);

	// Remove jags in precision recall curve
	float maxPrecision = -1;
	for(std::vector<PrecisionRecallPoint>::reverse_iterator pr = _data.rbegin(); pr != _data.rend(); pr++) {
		pr->precision = max(maxPrecision, pr->precision);
		maxPrecision = max(pr->precision, maxPrecision);
	}

	// Compute average precision as area under the curve
	_averagePrecision = 0.0;
	for(std::vector<PrecisionRecallPoint>::iterator pr = _data.begin() + 1, prPrev = _data.begin(); pr != _data.end(); pr++, prPrev++) {
		float xdiff = pr->recall - prPrev->recall;
		float ydiff = pr->precision - prPrev->precision;

		_averagePrecision += xdiff * prPrev->precision + xdiff * ydiff / 2.0;
	}
}

void PrecisionRecall::save(const char* filename) const
{
	std::ofstream f(filename);

	if(f.bad()) throw std::runtime_error("ERROR: Could not open file for writing");

	f << "# precision recall threshold\n";
	for(std::vector<PrecisionRecallPoint>::const_iterator pr = _data.begin(); pr != _data.end(); pr++) {
		f << pr->precision << " " << pr->recall << " " << pr->threshold << "\n";
	}
}

double  PrecisionRecall::getBestThreshold() const
{
	double bestFMeasure = -1, bestThreshold = -1;
	for(std::vector<PrecisionRecallPoint>::const_iterator pr = _data.begin(); pr != _data.end(); pr++) {
		double fMeasure = (pr->precision * pr->recall) / (pr->precision + pr->recall);

		if(fMeasure > bestFMeasure) {
			bestFMeasure = fMeasure;
			bestThreshold = pr->threshold;
		}
	}	

	return bestThreshold;
}
