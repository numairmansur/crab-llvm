#pragma once

#include "clam/HeapAbstraction.hh"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/ImmutableSet.h"
#include "llvm/ADT/StringRef.h"

#include <unordered_map>

// forward declarations
namespace sea_dsa {
class GlobalAnalysis;
class Node;
class Cell;
class AllocWrapInfo;
class ShadowMem;
} // namespace sea_dsa

namespace llvm {
class DataLayout;
class CallGraph;
class TargetLibraryInfo;
} // namespace llvm

namespace clam {
/*
 * Wrapper for sea-dsa (https://github.com/seahorn/sea-dsa)
 */
class LegacySeaDsaHeapAbstraction : public HeapAbstraction {

public:
  using typename HeapAbstraction::RegionId;
  using typename HeapAbstraction::RegionVec;

private:
  // XXX: We should use sea_dsa::Graph::SetFactory.
  // We copy here the definition of sea_dsa::Graph::SetFactory so
  // that we don't need to include Graph.hh
  using Set = llvm::ImmutableSet<llvm::Type *>;
  using SetFactory = typename Set::Factory;
  using region_bool_t = std::pair<Region, bool>;
  using callsite_map_t =
      llvm::DenseMap<const llvm::CallInst *, std::vector<region_bool_t>>;

  Region mkRegion(const sea_dsa::Cell &c, RegionInfo ri);
  RegionId getId(const sea_dsa::Cell &c);

  // compute and cache the set of read, mod and new nodes of a whole
  // function such that mod nodes are a subset of the read nodes and
  // the new nodes are disjoint from mod nodes.
  void computeReadModNewNodes(const llvm::Function &f);

  // Compute and cache the set of read, mod and new nodes of a
  // callsite such that mod nodes are a subset of the read nodes and
  // the new nodes are disjoint from mod nodes.
  void computeReadModNewNodesFromCallSite(const llvm::CallInst &I,
                                          callsite_map_t &accessed,
                                          callsite_map_t &mods,
                                          callsite_map_t &news);
  
  const llvm::Value *getSingleton(RegionId region) const;

public:
  LegacySeaDsaHeapAbstraction(const llvm::Module &M, llvm::CallGraph &cg,
                              const llvm::DataLayout &dl,
                              const llvm::TargetLibraryInfo &tli,
                              const sea_dsa::AllocWrapInfo &alloc_wrap_info,
                              bool is_context_sensitive,
                              bool disambiguate_unknown = false,
                              bool disambiguate_ptr_cast = false,
                              bool disambiguate_external = false);

  ~LegacySeaDsaHeapAbstraction();

  HeapAbstraction::ClassId getClassId() const {
    return HeapAbstraction::ClassId::SEA_DSA;
  }
  
  virtual Region getRegion(const llvm::Function &F,
                             const llvm::Instruction *I, const llvm::Value *V) override;

  virtual RegionVec getAccessedRegions(const llvm::Function &F) override;

  virtual RegionVec getOnlyReadRegions(const llvm::Function &F) override;

  virtual RegionVec getModifiedRegions(const llvm::Function &F) override;

  virtual RegionVec getNewRegions(const llvm::Function &F) override;

  virtual RegionVec getAccessedRegions(const llvm::CallInst &I) override;

  virtual RegionVec getOnlyReadRegions(const llvm::CallInst &I) override;

  virtual RegionVec getModifiedRegions(const llvm::CallInst &I) override;

  virtual RegionVec getNewRegions(const llvm::CallInst &I) override;

  virtual llvm::StringRef getName() const override {
    return "LegacySeaDsaHeapAbstraction";
  }

private:
  const llvm::Module &m_m;
  const llvm::DataLayout &m_dl;
  sea_dsa::GlobalAnalysis *m_dsa;
  SetFactory *m_fac;
  /// map from Node to id
  llvm::DenseMap<const sea_dsa::Node *, RegionId> m_node_ids;
  /// reverse map
  std::unordered_map<RegionId, const sea_dsa::Node *> m_rev_node_ids;
  RegionId m_max_id;
  bool m_disambiguate_unknown;
  bool m_disambiguate_ptr_cast;
  bool m_disambiguate_external;

  llvm::DenseMap<const llvm::Function *, RegionVec> m_func_accessed;
  llvm::DenseMap<const llvm::Function *, RegionVec> m_func_mods;
  llvm::DenseMap<const llvm::Function *, RegionVec> m_func_news;
  llvm::DenseMap<const llvm::CallInst *, RegionVec> m_callsite_accessed;
  llvm::DenseMap<const llvm::CallInst *, RegionVec> m_callsite_mods;
  llvm::DenseMap<const llvm::CallInst *, RegionVec> m_callsite_news;
};

using SeaDsaHeapAbstraction = LegacySeaDsaHeapAbstraction;
} // end namespace clam
