/*
 * aligner_seed_mm.h
 */

#ifndef ALIGNER_SEED_MM_H_
#define ALIGNER_SEED_MM_H_

#include <utility>
#include <vector>
#include "aligner.h"
#include "hit.h"
#include "row_chaser.h"
#include "range_chaser.h"
#include "aligner_metrics.h"

/**
 * Concrete factory class for constructing unpaired exact aligners.
 */
class UnpairedSeedAlignerV1Factory : public AlignerFactory {

	typedef RangeSourceDriver<EbwtRangeSource> TRangeSrcDr;
	typedef std::vector<TRangeSrcDr*> TRangeSrcDrPtrVec;
	typedef CostAwareRangeSourceDriver<EbwtRangeSource> TCostAwareRangeSrcDr;

public:
	UnpairedSeedAlignerV1Factory(
			Ebwt<String<Dna> >& ebwtFw,
			Ebwt<String<Dna> >* ebwtBw,
			bool doFw,
			bool doRc,
			uint32_t seedMms,
			uint32_t seedLen,
			int qualCutoff,
			int maxBts,
			HitSink& sink,
			const HitSinkPerThreadFactory& sinkPtFactory,
			RangeCache* cacheFw,
			RangeCache* cacheBw,
			uint32_t cacheLimit,
			vector<String<Dna5> >& os,
			bool maqPenalty,
			bool qualOrder,
			bool strandFix,
			bool rangeMode,
			bool verbose,
			uint32_t seed,
			AlignerMetrics *metrics) :
			ebwtFw_(ebwtFw),
			ebwtBw_(ebwtBw),
			doFw_(doFw), doRc_(doRc),
			seedMms_(seedMms),
			seedLen_(seedLen),
			qualCutoff_(qualCutoff),
			maxBts_(maxBts),
			sink_(sink),
			sinkPtFactory_(sinkPtFactory),
			cacheFw_(cacheFw),
			cacheBw_(cacheBw),
			cacheLimit_(cacheLimit),
			os_(os),
			strandFix_(strandFix),
			maqPenalty_(maqPenalty),
			qualOrder_(qualOrder),
			rangeMode_(rangeMode),
			verbose_(verbose),
			seed_(seed),
			metrics_(metrics)
	{
		assert(ebwtFw.isInMemory());
	}

	/**
	 * Create a new UnpairedExactAlignerV1s.
	 */
	virtual Aligner* create() const {
		HitSinkPerThread* sinkPt = sinkPtFactory_.create();
		EbwtSearchParams<String<Dna> >* params =
			new EbwtSearchParams<String<Dna> >(*sinkPt, os_, true, true, true, rangeMode_);
		AllocOnlyPool<Branch> *bpool = new AllocOnlyPool<Branch>((1 << 20), "branch");
		RangeStatePool *rpool = new RangeStatePool(1 * 1024 * 1024);
		AllocOnlyPool<Edit> *epool = new AllocOnlyPool<Edit>((1 << 20), "edit");
		int *btCnt = new int[1];
		*btCnt = maxBts_;

		TRangeSrcDrPtrVec *drVec = new TRangeSrcDrPtrVec();
		if(seedMms_ == 0) {
			EbwtRangeSource *rFw_Bw = new EbwtRangeSource(
				 ebwtBw_, true,  qualCutoff_, true, false, seed_, false, false, maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSource *rRc_Fw = new EbwtRangeSource(
				&ebwtFw_, false, qualCutoff_, true, false, seed_, false, false, maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSourceDriver * driverFw = new EbwtRangeSourceDriver(
				*params, rFw_Bw, true, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft (not applicable)
				PIN_TO_SEED_EDGE, // whole alignment is unrevisitable
				PIN_TO_SEED_EDGE, // "
				PIN_TO_SEED_EDGE, // "
				PIN_TO_SEED_EDGE, // "
				os_, verbose_, seed_, true, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
			EbwtRangeSourceDriver * driverRc = new EbwtRangeSourceDriver(
				*params, rRc_Fw, false, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft (not applicable)
				PIN_TO_SEED_EDGE, // whole alignment is unrevisitable
				PIN_TO_SEED_EDGE, // "
				PIN_TO_SEED_EDGE, // "
				PIN_TO_SEED_EDGE, // "
				os_, verbose_, seed_, true, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
			if(doFw_) drVec->push_back(driverFw);
			if(doRc_) drVec->push_back(driverRc);

		} else if(seedMms_ == 1) {

			bool fw = true;
			bool mate1 = true;

			EbwtRangeSource *rFw_Bw = new EbwtRangeSource(
				 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSourceFactory *rFw_BwSeed = new EbwtRangeSourceFactory(
				 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSource *rFw_FwSeedGen = new EbwtRangeSource(
				&ebwtFw_, fw,  qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_, metrics_);

			EbwtRangeSourceDriver * drFw_Bw = new EbwtRangeSourceDriver(
				*params, rFw_Bw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft
				PIN_TO_HI_HALF_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
			EbwtRangeSourceDriverFactory * drFw_BwSeed = new EbwtRangeSourceDriverFactory(
				*params, rFw_BwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
			EbwtRangeSourceDriver * drFw_FwSeedGen = new EbwtRangeSourceDriver(
				*params, rFw_FwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				false,      // nudgeLeft
				PIN_TO_HI_HALF_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
			EbwtSeededRangeSourceDriver * drFw_Seed = new EbwtSeededRangeSourceDriver(
				drFw_BwSeed, drFw_FwSeedGen, fw, seedLen_, verbose_, mate1);

			fw = false;

			EbwtRangeSource *rRc_Fw = new EbwtRangeSource(
				&ebwtFw_, fw, qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSourceFactory *rRc_FwSeed = new EbwtRangeSourceFactory(
				&ebwtFw_, fw, qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSource *rRc_BwSeedGen = new EbwtRangeSource(
				 ebwtBw_, fw, qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_, metrics_);

			EbwtRangeSourceDriver * drRc_Fw = new EbwtRangeSourceDriver(
				*params, rRc_Fw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft
				PIN_TO_HI_HALF_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
			EbwtRangeSourceDriverFactory * drRc_FwSeed = new EbwtRangeSourceDriverFactory(
				*params, rRc_FwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
			EbwtRangeSourceDriver * drRc_BwSeedGen = new EbwtRangeSourceDriver(
				*params, rRc_BwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				false,      // nudgeLeft
				PIN_TO_HI_HALF_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
			EbwtSeededRangeSourceDriver * drRc_Seed = new EbwtSeededRangeSourceDriver(
				drRc_FwSeed, drRc_BwSeedGen, fw, seedLen_, verbose_, true);

			if(doFw_) {
				drVec->push_back(drFw_Bw);
				drVec->push_back(drFw_Seed);
			}
			if(doRc_) {
				drVec->push_back(drRc_Fw);
				drVec->push_back(drRc_Seed);
			}
		} else if(seedMms_ > 1) {

			bool fw = true;
			bool mate1 = true;
			bool two = (seedMms_ == 2);

			EbwtRangeSource *rFw_Bw = new EbwtRangeSource(
				 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSourceFactory *rFw_BwSeed = new EbwtRangeSourceFactory(
				 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSource *rFw_FwSeedGen = new EbwtRangeSource(
				&ebwtFw_, fw,  qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSource *rFw_BwHalf = new EbwtRangeSource(
				 ebwtBw_, fw,  qualCutoff_, false, verbose_, seed_, true,  false, maqPenalty_, qualOrder_, metrics_);

			EbwtRangeSourceDriver * drFw_Bw = new EbwtRangeSourceDriver(
				*params, rFw_Bw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft
				PIN_TO_HI_HALF_EDGE,
				PIN_TO_HI_HALF_EDGE,
				two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
			EbwtRangeSourceDriverFactory * drFw_BwSeed = new EbwtRangeSourceDriverFactory(
				*params, rFw_BwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
			EbwtRangeSourceDriver * drFw_FwSeedGen = new EbwtRangeSourceDriver(
				*params, rFw_FwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				false,      // nudgeLeft
				PIN_TO_HI_HALF_EDGE,
				PIN_TO_HI_HALF_EDGE,
				two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
			EbwtSeededRangeSourceDriver * drFw_Seed = new EbwtSeededRangeSourceDriver(
				drFw_BwSeed, drFw_FwSeedGen, fw, seedLen_, verbose_, mate1);
			EbwtRangeSourceDriver * drFw_BwHalf = new EbwtRangeSourceDriver(
				*params, rFw_BwHalf, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft
				PIN_TO_BEGINNING,    // nothing's unrevisitable
				two ? PIN_TO_HI_HALF_EDGE : PIN_TO_BEGINNING,
				two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);

			fw = false;

			EbwtRangeSource *rRc_Fw = new EbwtRangeSource(
				&ebwtFw_, fw, qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSourceFactory *rRc_FwSeed = new EbwtRangeSourceFactory(
				&ebwtFw_, fw, qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSource *rRc_BwSeedGen = new EbwtRangeSource(
				 ebwtBw_, fw, qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_, metrics_);
			EbwtRangeSource *rRc_FwHalf = new EbwtRangeSource(
				&ebwtFw_, fw, qualCutoff_, false, verbose_, seed_, true,  false, maqPenalty_, qualOrder_, metrics_);

			EbwtRangeSourceDriver * drRc_Fw = new EbwtRangeSourceDriver(
				*params, rRc_Fw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft
				PIN_TO_HI_HALF_EDGE,
				PIN_TO_HI_HALF_EDGE,
				two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
			EbwtRangeSourceDriverFactory * drRc_FwSeed = new EbwtRangeSourceDriverFactory(
				*params, rRc_FwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
			EbwtRangeSourceDriver * drRc_BwSeedGen = new EbwtRangeSourceDriver(
				*params, rRc_BwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				false,      // nudgeLeft
				PIN_TO_HI_HALF_EDGE,
				PIN_TO_HI_HALF_EDGE,
				two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
			EbwtSeededRangeSourceDriver * drRc_Seed = new EbwtSeededRangeSourceDriver(
				drRc_FwSeed, drRc_BwSeedGen, fw, seedLen_, verbose_, true);
			EbwtRangeSourceDriver * drRc_FwHalf = new EbwtRangeSourceDriver(
				*params, rRc_FwHalf, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
				seedLen_,   // seedLen
				true,       // nudgeLeft
				PIN_TO_BEGINNING,    // nothing's unrevisitable
				two ? PIN_TO_HI_HALF_EDGE : PIN_TO_BEGINNING,
				two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
				PIN_TO_SEED_EDGE,
				os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);

			if(doFw_) {
				drVec->push_back(drFw_Bw);
				drVec->push_back(drFw_Seed);
				drVec->push_back(drFw_BwHalf);
			}
			if(doRc_) {
				drVec->push_back(drRc_Fw);
				drVec->push_back(drRc_Seed);
				drVec->push_back(drRc_FwHalf);
			}
		} else {
			cerr << "Unsupported --stateful mode: " << seedMms_ << endl;
		}
		TCostAwareRangeSrcDr* dr = new TCostAwareRangeSrcDr(seed_, strandFix_, drVec, verbose_);

		// Set up a RangeChaser
		RangeChaser<String<Dna> > *rchase =
			new RangeChaser<String<Dna> >(seed_, cacheLimit_, cacheFw_, cacheBw_, metrics_);

		return new UnpairedAlignerV2<EbwtRangeSource>(
			params, dr, rchase,
			sink_, sinkPtFactory_, sinkPt, os_, rangeMode_, verbose_,
			seed_, maxBts_, bpool, rpool, epool, btCnt, metrics_);
	}

private:
	Ebwt<String<Dna> >& ebwtFw_;
	Ebwt<String<Dna> >* ebwtBw_;
	bool doFw_;
	bool doRc_;
	const uint32_t seedMms_;
	const uint32_t seedLen_;
	const int qualCutoff_;
	const int maxBts_;
	HitSink& sink_;
	const HitSinkPerThreadFactory& sinkPtFactory_;
	RangeCache *cacheFw_;
	RangeCache *cacheBw_;
	const uint32_t cacheLimit_;
	vector<String<Dna5> >& os_;
	bool strandFix_;
	bool maqPenalty_;
	bool qualOrder_;
	bool rangeMode_;
	bool verbose_;
	uint32_t seed_;
	AlignerMetrics *metrics_;
};

/**
 * Concrete factory class for constructing unpaired exact aligners.
 */
class PairedSeedAlignerV1Factory : public AlignerFactory {
	typedef RangeSourceDriver<EbwtRangeSource> TRangeSrcDr;
	typedef std::vector<TRangeSrcDr*> TRangeSrcDrPtrVec;
	typedef CostAwareRangeSourceDriver<EbwtRangeSource> TCostAwareRangeSrcDr;
public:
	PairedSeedAlignerV1Factory(
			Ebwt<String<Dna> >& ebwtFw,
			Ebwt<String<Dna> >* ebwtBw,
			bool doFw,
			bool doRc,
			uint32_t seedMms,
			uint32_t seedLen,
			int qualCutoff,
			int maxBts,
			HitSink& sink,
			const HitSinkPerThreadFactory& sinkPtFactory,
			bool mate1fw,
			bool mate2fw,
			uint32_t peInner,
			uint32_t peOuter,
			bool dontReconcile,
			uint32_t symCeil,
			uint32_t mixedThresh,
			uint32_t mixedAttemptLim,
			RangeCache* cacheFw,
			RangeCache* cacheBw,
			uint32_t cacheLimit,
			BitPairReference* refs,
			vector<String<Dna5> >& os,
			bool maqPenalty,
			bool qualOrder,
			bool strandFix,
			bool rangeMode,
			bool verbose,
			uint32_t seed) :
			ebwtFw_(ebwtFw),
			ebwtBw_(ebwtBw),
			doFw_(doFw),
			doRc_(doRc),
			seedMms_(seedMms),
			seedLen_(seedLen),
			qualCutoff_(qualCutoff),
			maxBts_(maxBts),
			sink_(sink),
			sinkPtFactory_(sinkPtFactory),
			mate1fw_(mate1fw),
			mate2fw_(mate2fw),
			peInner_(peInner),
			peOuter_(peOuter),
			dontReconcile_(dontReconcile),
			symCeil_(symCeil),
			mixedThresh_(mixedThresh),
			mixedAttemptLim_(mixedAttemptLim),
			cacheFw_(cacheFw),
			cacheBw_(cacheBw),
			cacheLimit_(cacheLimit),
			refs_(refs), os_(os),
			maqPenalty_(maqPenalty),
			qualOrder_(qualOrder),
			strandFix_(strandFix),
			rangeMode_(rangeMode),
			verbose_(verbose),
			seed_(seed)
	{
		assert(ebwtFw.isInMemory());
		assert(ebwtBw->isInMemory());
	}

	/**
	 * Create a new UnpairedExactAlignerV1s.
	 */
	virtual Aligner* create() const {
		HitSinkPerThread* sinkPt = sinkPtFactory_.createMult(2);
		EbwtSearchParams<String<Dna> >* params =
			new EbwtSearchParams<String<Dna> >(*sinkPt, os_, true, true, true, rangeMode_);
		RefAligner<String<Dna5> >* refAligner = NULL;
		AllocOnlyPool<Branch> *bpool = new AllocOnlyPool<Branch>((1 << 20), "branch");
		RangeStatePool *rpool = new RangeStatePool(1 * 1024 * 1024);
		AllocOnlyPool<Edit> *epool = new AllocOnlyPool<Edit>((1 << 20), "edit");
		int *btCnt = new int[1];
		*btCnt = maxBts_;
		if(seedMms_ == 0) {
			refAligner = new Seed0RefAligner<String<Dna5> >(verbose_, seedLen_, qualCutoff_, maqPenalty_);
		} else if(seedMms_ == 1) {
			refAligner = new Seed1RefAligner<String<Dna5> >(verbose_, seedLen_, qualCutoff_, maqPenalty_);
		} else if(seedMms_ == 2) {
			refAligner = new Seed2RefAligner<String<Dna5> >(verbose_, seedLen_, qualCutoff_, maqPenalty_);
		} else {
			refAligner = new Seed3RefAligner<String<Dna5> >(verbose_, seedLen_, qualCutoff_, maqPenalty_);
		}
		bool do1Fw = true;
		bool do1Rc = true;
		bool do2Fw = true;
		bool do2Rc = true;
		if(!doFw_) {
			if(mate1fw_) do1Fw = false;
			else         do1Rc = false;
			if(mate2fw_) do2Fw = false;
			else         do2Rc = false;
		}
		if(!doRc_) {
			if(mate1fw_) do1Rc = false;
			else         do1Fw = false;
			if(mate2fw_) do2Rc = false;
			else         do2Fw = false;
		}
		TRangeSrcDrPtrVec *dr1FwVec = new TRangeSrcDrPtrVec();
		TRangeSrcDrPtrVec *dr1RcVec = new TRangeSrcDrPtrVec();
		TRangeSrcDrPtrVec *dr2FwVec = new TRangeSrcDrPtrVec();
		TRangeSrcDrPtrVec *dr2RcVec = new TRangeSrcDrPtrVec();
		if(seedMms_ == 0) {
			if(do1Fw) {
				bool mate1 = true;
				bool fw = true;
				EbwtRangeSource *r1Fw_Bw = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, true, false, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceDriver *dr1Fw_Bw = new EbwtRangeSourceDriver(
					*params, r1Fw_Bw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft (not applicable)
					PIN_TO_SEED_EDGE, // whole alignment is unrevisitable
					PIN_TO_SEED_EDGE, // "
					PIN_TO_SEED_EDGE, // "
					PIN_TO_SEED_EDGE, // "
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				dr1FwVec->push_back(dr1Fw_Bw);
			}
			if(do2Fw) {
				bool mate1 = false;
				bool fw = true;
				EbwtRangeSource *r2Fw_Bw = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, true, false, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceDriver *dr2Fw_Bw = new EbwtRangeSourceDriver(
					*params, r2Fw_Bw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft (not applicable)
					PIN_TO_SEED_EDGE, // whole alignment is unrevisitable
					PIN_TO_SEED_EDGE, // "
					PIN_TO_SEED_EDGE, // "
					PIN_TO_SEED_EDGE, // "
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				dr2FwVec->push_back(dr2Fw_Bw);
			}
			if(do1Rc) {
				bool mate1 = true;
				bool fw = false;
				EbwtRangeSource *r1Rc_Fw = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, true, verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceDriver *dr1Rc_Fw = new EbwtRangeSourceDriver(
					*params, r1Rc_Fw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft (not applicable)
					PIN_TO_SEED_EDGE, // whole alignment is unrevisitable
					PIN_TO_SEED_EDGE, // "
					PIN_TO_SEED_EDGE, // "
					PIN_TO_SEED_EDGE, // "
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				dr1RcVec->push_back(dr1Rc_Fw);
			}
			if(do2Rc) {
				bool mate1 = false;
				bool fw = false;
				EbwtRangeSource *r2Rc_Fw = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, true, verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceDriver *dr2Rc_Fw = new EbwtRangeSourceDriver(
					*params, r2Rc_Fw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft (not applicable)
					PIN_TO_SEED_EDGE, // whole alignment is unrevisitable
					PIN_TO_SEED_EDGE, // "
					PIN_TO_SEED_EDGE, // "
					PIN_TO_SEED_EDGE, // "
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				dr2RcVec->push_back(dr2Rc_Fw);
			}
		} else if(seedMms_ == 1) {
			if(do1Fw) {
				bool mate1 = true;
				bool fw = true;
				EbwtRangeSource *rFw_Bw = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceFactory *rFw_BwSeed = new EbwtRangeSourceFactory(
					 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSource *rFw_FwSeedGen = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_);
				EbwtRangeSourceDriver * drFw_Bw = new EbwtRangeSourceDriver(
					*params, rFw_Bw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtRangeSourceDriverFactory * drFw_BwSeed = new EbwtRangeSourceDriverFactory(
					*params, rFw_BwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtRangeSourceDriver * drFw_FwSeedGen = new EbwtRangeSourceDriver(
					*params, rFw_FwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					false,      // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtSeededRangeSourceDriver * drFw_Seed = new EbwtSeededRangeSourceDriver(
					drFw_BwSeed, drFw_FwSeedGen, fw, seedLen_, verbose_, mate1);
				dr1FwVec->push_back(drFw_Bw);
				dr1FwVec->push_back(drFw_Seed);
			}
			if(do2Fw) {
				bool mate1 = false;
				bool fw = true;
				EbwtRangeSource *rFw_Bw = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceFactory *rFw_BwSeed = new EbwtRangeSourceFactory(
					 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSource *rFw_FwSeedGen = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_);
				EbwtRangeSourceDriver * drFw_Bw = new EbwtRangeSourceDriver(
					*params, rFw_Bw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtRangeSourceDriverFactory * drFw_BwSeed = new EbwtRangeSourceDriverFactory(
					*params, rFw_BwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtRangeSourceDriver * drFw_FwSeedGen = new EbwtRangeSourceDriver(
					*params, rFw_FwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					false,      // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtSeededRangeSourceDriver * drFw_Seed = new EbwtSeededRangeSourceDriver(
					drFw_BwSeed, drFw_FwSeedGen, fw, seedLen_, verbose_, mate1);
				dr2FwVec->push_back(drFw_Bw);
				dr2FwVec->push_back(drFw_Seed);
			}
			if(do1Rc) {
				bool mate1 = true;
				bool fw = false;
				EbwtRangeSource *rRc_Fw = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceFactory *rRc_FwSeed = new EbwtRangeSourceFactory(
					&ebwtFw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSource *rRc_BwSeedGen = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_);
				EbwtRangeSourceDriver * drRc_Fw = new EbwtRangeSourceDriver(
					*params, rRc_Fw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtRangeSourceDriverFactory * drRc_FwSeed = new EbwtRangeSourceDriverFactory(
					*params, rRc_FwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtRangeSourceDriver * drRc_BwSeedGen = new EbwtRangeSourceDriver(
					*params, rRc_BwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					false,      // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtSeededRangeSourceDriver * drRc_Seed = new EbwtSeededRangeSourceDriver(
					drRc_FwSeed, drRc_BwSeedGen, fw, seedLen_, verbose_, mate1);
				dr1RcVec->push_back(drRc_Fw);
				dr1RcVec->push_back(drRc_Seed);
			}
			if(do2Rc) {
				bool mate1 = false;
				bool fw = false;
				EbwtRangeSource *rRc_Fw = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceFactory *rRc_FwSeed = new EbwtRangeSourceFactory(
					&ebwtFw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSource *rRc_BwSeedGen = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_);
				EbwtRangeSourceDriver * drRc_Fw = new EbwtRangeSourceDriver(
					*params, rRc_Fw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtRangeSourceDriverFactory * drRc_FwSeed = new EbwtRangeSourceDriverFactory(
					*params, rRc_FwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtRangeSourceDriver * drRc_BwSeedGen = new EbwtRangeSourceDriver(
					*params, rRc_BwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					false,      // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, NULL); // no backtrack limit for -n 1/2
				EbwtSeededRangeSourceDriver * drRc_Seed = new EbwtSeededRangeSourceDriver(
					drRc_FwSeed, drRc_BwSeedGen, fw, seedLen_, verbose_, mate1);
				dr2RcVec->push_back(drRc_Fw);
				dr2RcVec->push_back(drRc_Seed);
			}
		} else if(seedMms_ > 1) {
			bool two = seedMms_ == 2;
			if(do1Fw) {
				bool mate1 = true;
				bool fw = true;
				EbwtRangeSource *rFw_Bw = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceFactory *rFw_BwSeed = new EbwtRangeSourceFactory(
					 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSource *rFw_FwSeedGen = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_);
				EbwtRangeSource *rFw_BwHalf = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, false, verbose_, seed_, true,  false, maqPenalty_, qualOrder_);
				EbwtRangeSourceDriver * drFw_Bw = new EbwtRangeSourceDriver(
					*params, rFw_Bw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_HI_HALF_EDGE,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtRangeSourceDriverFactory * drFw_BwSeed = new EbwtRangeSourceDriverFactory(
					*params, rFw_BwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtRangeSourceDriver * drFw_FwSeedGen = new EbwtRangeSourceDriver(
					*params, rFw_FwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					false,      // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_HI_HALF_EDGE,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtSeededRangeSourceDriver * drFw_Seed = new EbwtSeededRangeSourceDriver(
					drFw_BwSeed, drFw_FwSeedGen, fw, seedLen_, verbose_, mate1);
				EbwtRangeSourceDriver * drFw_BwHalf = new EbwtRangeSourceDriver(
					*params, rFw_BwHalf, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_BEGINNING,    // nothing's unrevisitable
					two ? PIN_TO_HI_HALF_EDGE : PIN_TO_BEGINNING,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				dr1FwVec->push_back(drFw_Bw);
				dr1FwVec->push_back(drFw_Seed);
				dr1FwVec->push_back(drFw_BwHalf);
			}
			if(do2Fw) {
				bool mate1 = false;
				bool fw = true;
				EbwtRangeSource *rFw_Bw = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceFactory *rFw_BwSeed = new EbwtRangeSourceFactory(
					 ebwtBw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSource *rFw_FwSeedGen = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_);
				EbwtRangeSource *rFw_BwHalf = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, false, verbose_, seed_, true,  false, maqPenalty_, qualOrder_);

				EbwtRangeSourceDriver * drFw_Bw = new EbwtRangeSourceDriver(
					*params, rFw_Bw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_HI_HALF_EDGE,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtRangeSourceDriverFactory * drFw_BwSeed = new EbwtRangeSourceDriverFactory(
					*params, rFw_BwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtRangeSourceDriver * drFw_FwSeedGen = new EbwtRangeSourceDriver(
					*params, rFw_FwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					false,      // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_HI_HALF_EDGE,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtSeededRangeSourceDriver * drFw_Seed = new EbwtSeededRangeSourceDriver(
					drFw_BwSeed, drFw_FwSeedGen, fw, seedLen_, verbose_, mate1);
				EbwtRangeSourceDriver * drFw_BwHalf = new EbwtRangeSourceDriver(
					*params, rFw_BwHalf, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_BEGINNING,    // nothing's unrevisitable
					two ? PIN_TO_HI_HALF_EDGE : PIN_TO_BEGINNING,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				dr2FwVec->push_back(drFw_Bw);
				dr2FwVec->push_back(drFw_Seed);
				dr2FwVec->push_back(drFw_BwHalf);
			}
			if(do1Rc) {
				bool mate1 = true;
				bool fw = false;
				EbwtRangeSource *rRc_Fw = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceFactory *rRc_FwSeed = new EbwtRangeSourceFactory(
					&ebwtFw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSource *rRc_BwSeedGen = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_);
				EbwtRangeSource *rRc_FwHalf = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, false, verbose_, seed_, true,  false, maqPenalty_, qualOrder_);

				EbwtRangeSourceDriver * drRc_Fw = new EbwtRangeSourceDriver(
					*params, rRc_Fw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_HI_HALF_EDGE,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtRangeSourceDriverFactory * drRc_FwSeed = new EbwtRangeSourceDriverFactory(
					*params, rRc_FwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtRangeSourceDriver * drRc_BwSeedGen = new EbwtRangeSourceDriver(
					*params, rRc_BwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					false,      // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_HI_HALF_EDGE,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtSeededRangeSourceDriver * drRc_Seed = new EbwtSeededRangeSourceDriver(
					drRc_FwSeed, drRc_BwSeedGen, fw, seedLen_, verbose_, mate1);
				EbwtRangeSourceDriver * drRc_FwHalf = new EbwtRangeSourceDriver(
					*params, rRc_FwHalf, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_BEGINNING,    // nothing's unrevisitable
					two ? PIN_TO_HI_HALF_EDGE : PIN_TO_BEGINNING,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				dr1RcVec->push_back(drRc_Fw);
				dr1RcVec->push_back(drRc_Seed);
				dr1RcVec->push_back(drRc_FwHalf);
			}
			if(do2Rc) {
				bool mate1 = false;
				bool fw = false;
				EbwtRangeSource *rRc_Fw = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSourceFactory *rRc_FwSeed = new EbwtRangeSourceFactory(
					&ebwtFw_, fw,  qualCutoff_, true,  verbose_, seed_, false, false, maqPenalty_, qualOrder_);
				EbwtRangeSource *rRc_BwSeedGen = new EbwtRangeSource(
					 ebwtBw_, fw,  qualCutoff_, false, verbose_, seed_, false, true,  maqPenalty_, qualOrder_);
				EbwtRangeSource *rRc_FwHalf = new EbwtRangeSource(
					&ebwtFw_, fw,  qualCutoff_, false, verbose_, seed_, true,  false, maqPenalty_, qualOrder_);

				EbwtRangeSourceDriver * drRc_Fw = new EbwtRangeSourceDriver(
					*params, rRc_Fw, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_HI_HALF_EDGE,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtRangeSourceDriverFactory * drRc_FwSeed = new EbwtRangeSourceDriverFactory(
					*params, rRc_FwSeed, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtRangeSourceDriver * drRc_BwSeedGen = new EbwtRangeSourceDriver(
					*params, rRc_BwSeedGen, fw, true, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					false,      // nudgeLeft
					PIN_TO_HI_HALF_EDGE,
					PIN_TO_HI_HALF_EDGE,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				EbwtSeededRangeSourceDriver * drRc_Seed = new EbwtSeededRangeSourceDriver(
					drRc_FwSeed, drRc_BwSeedGen, fw, seedLen_, verbose_, mate1);
				EbwtRangeSourceDriver * drRc_FwHalf = new EbwtRangeSourceDriver(
					*params, rRc_FwHalf, fw, false, maqPenalty_, qualOrder_, sink_, sinkPt,
					seedLen_,   // seedLen
					true,       // nudgeLeft
					PIN_TO_BEGINNING,    // nothing's unrevisitable
					two ? PIN_TO_HI_HALF_EDGE : PIN_TO_BEGINNING,
					two ? PIN_TO_SEED_EDGE : PIN_TO_HI_HALF_EDGE,
					PIN_TO_SEED_EDGE,
					os_, verbose_, seed_, mate1, bpool, rpool, epool, btCnt);
				dr2RcVec->push_back(drRc_Fw);
				dr2RcVec->push_back(drRc_Seed);
				dr2RcVec->push_back(drRc_FwHalf);
			}
		} else {
			cerr << "Unsupported --stateful mode: " << seedMms_ << endl;
		}
		TCostAwareRangeSrcDr* driver1Fw = new TCostAwareRangeSrcDr(seed_, strandFix_, dr1FwVec, verbose_);
		TCostAwareRangeSrcDr* driver1Rc = new TCostAwareRangeSrcDr(seed_, strandFix_, dr1RcVec, verbose_);
		TCostAwareRangeSrcDr* driver2Fw = new TCostAwareRangeSrcDr(seed_, strandFix_, dr2FwVec, verbose_);
		TCostAwareRangeSrcDr* driver2Rc = new TCostAwareRangeSrcDr(seed_, strandFix_, dr2RcVec, verbose_);

		// Set up a RangeChaser
		RangeChaser<String<Dna> > *rchase =
			new RangeChaser<String<Dna> >(seed_, cacheLimit_, cacheFw_, cacheBw_);

		return new PairedBWAlignerV1<EbwtRangeSource>(
			params,
			driver1Fw, driver1Rc, driver2Fw, driver2Rc, refAligner,
			rchase, sink_, sinkPtFactory_, sinkPt, mate1fw_, mate2fw_,
			peInner_, peOuter_, dontReconcile_, symCeil_, mixedThresh_,
			mixedAttemptLim_, refs_, rangeMode_, verbose_, seed_,
			maxBts_, bpool, rpool, epool, btCnt);
	}

private:
	Ebwt<String<Dna> >& ebwtFw_;
	Ebwt<String<Dna> >* ebwtBw_;
	const bool doFw_;
	const bool doRc_;
	const uint32_t seedMms_;
	const uint32_t seedLen_;
	const int qualCutoff_;
	const int maxBts_;
	HitSink& sink_;
	const HitSinkPerThreadFactory& sinkPtFactory_;
	const bool mate1fw_;
	const bool mate2fw_;
	const uint32_t peInner_;
	const uint32_t peOuter_;
	const bool dontReconcile_;
	const uint32_t symCeil_;
	const uint32_t mixedThresh_;
	const uint32_t mixedAttemptLim_;
	RangeCache *cacheFw_;
	RangeCache *cacheBw_;
	const uint32_t cacheLimit_;
	BitPairReference* refs_;
	vector<String<Dna5> >& os_;
	const bool maqPenalty_;
	const bool qualOrder_;
	const bool strandFix_;
	const bool rangeMode_;
	const bool verbose_;
	const uint32_t seed_;
};

#endif /* ALIGNER_SEED_MM_H_ */