#include "DSWP.hpp"
#include "SCCDAGPartition.hpp"

using namespace llvm;

static void partitionHeuristics (DSWPLoopDependenceInfo *LDI);

void DSWP::partitionSCCDAG (DSWPLoopDependenceInfo *LDI) {

  /*
   * Initial the partition structure with the merged SCCDAG
   */
  LDI->partitions.initialize(LDI->loopSCCDAG, &LDI->sccdagInfo, &LDI->liSummary, /*idealThreads=*/ 2);

  /*
   * Check if we can cluster SCCs.
   */
  if (!this->forceNoSCCPartition) {
    clusterSubloops(LDI);
  }

  /*
   * Assign SCCs that have no partition to their own partitions.
   */
  for (auto nodePair : LDI->loopSCCDAG->internalNodePairs()) {

    /*
     * Check if the current SCC can be removed (e.g., because it is due to induction variables).
     * If it is, then this SCC has already been assigned to every dependent partition.
     */
    auto currentSCC = nodePair.first;
    if (LDI->partitions.isRemovable(currentSCC)) continue ;

    /*
     * Check if the current SCC has been already assigned to a partition; if not, assign it to a new partition.
     */
    if (LDI->partitions.partitionOf(currentSCC) == nullptr) {
      LDI->partitions.addPartition(nodePair.first);
    }
  }

  /*
   * Print the initial partitions.
   */
  if (this->verbose >= Verbosity::Maximal) {
    errs() << "DSWP:  Before partitioning the SCCDAG\n";
    printPartitions(LDI);
  }

  /*
   * Check if we can cluster SCCs.
   */
  if (!this->forceNoSCCPartition) {

    /*
     * Decide the partition of the SCCDAG by merging the trivial partitions defined above.
     */
    partitionHeuristics(LDI);
  }

  /*
   * Print the partitioned SCCDAG.
   */
  if (this->verbose >= Verbosity::Maximal) {
    errs() << "DSWP:  After partitioning the SCCDAG\n";
    printPartitions(LDI);
  }

  return ;
}

void DSWP::mergeTrivialNodesInSCCDAG (DSWPLoopDependenceInfo *LDI) {

  /*
   * Print the current SCCDAG.
   */
  if (this->verbose >= Verbosity::Maximal) {
    errs() << "DSWP:  Before merging SCCs\n";
    printSCCs(LDI->loopSCCDAG);
  }

  /*
   * Merge SCCs.
   */
  mergeSingleSyntacticSugarInstrs(LDI);
  mergeBranchesWithoutOutgoingEdges(LDI);

  /*
   * Print the current SCCDAG.
   */
  if (this->verbose >= Verbosity::Maximal) {
    errs() << "DSWP:  After merging SCCs\n";
    printSCCs(LDI->loopSCCDAG);
  }

  return ;
}

void DSWP::mergeSingleSyntacticSugarInstrs (DSWPLoopDependenceInfo *LDI) {
  std::unordered_map<DGNode<SCC> *, std::set<DGNode<SCC> *> *> mergedToGroup;
  std::set<std::set<DGNode<SCC> *> *> singles;
  for (auto sccNode : LDI->loopSCCDAG->getNodes()) {
    auto scc = sccNode->getT();

    /*
     * Determine if node is a single syntactic sugar instruction with only one dependent SCC
     */
    if (scc->numInternalNodes() > 1) continue;
    auto I = scc->begin_internal_node_map()->first;
    if (!isa<PHINode>(I) && !isa<GetElementPtrInst>(I) && !isa<CastInst>(I)) continue;
    if (sccNode->numOutgoingEdges() != 1) continue;
    auto dependentNode = (*sccNode->begin_outgoing_edges())->getIncomingNode();

    /*
     * Determine the merged state of the single instruction node and its dependent
     * Merge the current merged nodes holding both of the above
     */
    bool mergedSCCNode = mergedToGroup.find(sccNode) != mergedToGroup.end();
    bool mergedDepNode = mergedToGroup.find(dependentNode) != mergedToGroup.end();
    if (mergedSCCNode && mergedDepNode) {

      /*
       * Combine the dependent node's merged group into that of the single instruction's merged group
       */
      auto depSet = mergedToGroup[dependentNode];
      for (auto node : *depSet) {
        mergedToGroup[sccNode]->insert(node);
        mergedToGroup[node] = mergedToGroup[sccNode];
      }
      singles.erase(depSet);
      delete depSet;
    } else if (mergedSCCNode) {
      mergedToGroup[sccNode]->insert(dependentNode);
      mergedToGroup[dependentNode] = mergedToGroup[sccNode];
    } else if (mergedDepNode) {
      mergedToGroup[dependentNode]->insert(sccNode);
      mergedToGroup[sccNode] = mergedToGroup[dependentNode];
    } else {
      auto nodes = new std::set<DGNode<SCC> *>({ sccNode, dependentNode });
      singles.insert(nodes);
      mergedToGroup[sccNode] = nodes;
      mergedToGroup[dependentNode] = nodes;
    }
  }

  for (auto sccNodes : singles) { 
    LDI->loopSCCDAG->mergeSCCs(*sccNodes);
    delete sccNodes;
  }
}

void DSWP::clusterSubloops (DSWPLoopDependenceInfo *LDI) {
  auto &li = LDI->liSummary;
  auto loop = li.bbToLoop[LDI->header];
  auto loopDepth = loop->depth;

  unordered_map<LoopSummary *, std::set<SCC *>> loopSets;
  for (auto sccNode : LDI->loopSCCDAG->getNodes()) {
    if (LDI->partitions.isRemovable(sccNode->getT())) continue;

    for (auto iNodePair : sccNode->getT()->internalNodePairs()) {
      auto bb = cast<Instruction>(iNodePair.first)->getParent();
      auto subL = li.bbToLoop[bb];
      auto subDepth = subL->depth;
      if (loopDepth >= subDepth) continue;

      while (subDepth - 1 > loopDepth) {
        subL = subL->parent;
        subDepth--;
      }
      loopSets[subL].insert(sccNode->getT());
      break;
    }
  }

  /*
   * Basic Heuristic: partition entire sub loops only if there is more than one
   */
  if (loopSets.size() == 1) return;
  for (auto loopSetPair : loopSets) {
    LDI->partitions.addPartition(loopSetPair.second);
  }
}

void DSWP::mergeBranchesWithoutOutgoingEdges (DSWPLoopDependenceInfo *LDI) {
  std::vector<DGNode<SCC> *> tailCmpBrs;
  for (auto sccNode : LDI->loopSCCDAG->getNodes()) {
    auto scc = sccNode->getT();
    if (sccNode->numIncomingEdges() == 0 || sccNode->numOutgoingEdges() > 0) continue ;

    bool allCmpOrBr = true;
    for (auto node : scc->getNodes()) {
      allCmpOrBr &= (isa<TerminatorInst>(node->getT()) || isa<CmpInst>(node->getT()));
    }
    if (allCmpOrBr) tailCmpBrs.push_back(sccNode);
  }

  /*
   * Merge trailing compare/branch scc into previous depth scc
   */
  for (auto tailSCC : tailCmpBrs) {
    std::set<DGNode<SCC> *> nodesToMerge = { tailSCC };
    nodesToMerge.insert(*LDI->loopSCCDAG->previousDepthNodes(tailSCC).begin());
    LDI->loopSCCDAG->mergeSCCs(nodesToMerge);
  }
}

void DSWP::addRemovableSCCsToStages (DSWPLoopDependenceInfo *LDI) {
  for (auto &stage : LDI->stages) {
    std::set<DGNode<SCC> *> visitedNodes;
    std::queue<DGNode<SCC> *> dependentSCCNodes;

    for (auto scc : stage->stageSCCs) {
      dependentSCCNodes.push(LDI->loopSCCDAG->fetchNode(scc));
    }

    while (!dependentSCCNodes.empty()) {
      auto depSCCNode = dependentSCCNodes.front();
      dependentSCCNodes.pop();

      for (auto sccEdge : depSCCNode->getIncomingEdges()) {
        auto fromSCCNode = sccEdge->getOutgoingNode();
        auto fromSCC = fromSCCNode->getT();
        if (visitedNodes.find(fromSCCNode) != visitedNodes.end()) continue;
        if (!LDI->partitions.isRemovable(fromSCC)) continue;

        stage->removableSCCs.insert(fromSCC);
        dependentSCCNodes.push(fromSCCNode);
        visitedNodes.insert(fromSCCNode);
      }
    }
  }
}

static void partitionHeuristics (DSWPLoopDependenceInfo *LDI) {

  /*
   * Collect all top level partitions
   */
  std::queue<SCCDAGPartition *> partToCheck;
  auto topLevelParts = LDI->partitions.topLevelPartitions();
  for (auto part : topLevelParts) partToCheck.push(part);

  /*
   * Merge partitions.
   */
  while (!partToCheck.empty()) {

    /*
     * Fetch the current partition.
     */
    auto partition = partToCheck.front();
    partToCheck.pop();

    /*
     * Check if the current partition has been already tagged to be removed (i.e., merged).
     */
    if (!LDI->partitions.isValidPartition(partition)) {
      continue;
    }
    partition->print(errs() << "DSWP:   CHECKING PARTITION:\n", "DSWP:   ");

    /*
     * Prioritize merge that best lowers overall cost without yielding a too costly partition
     */
    SCCDAGPartition *minPartition = nullptr;
    int32_t maxLoweredCost = 0;
    auto maxAllowedCost = LDI->partitions.maxPartitionCost();

    auto checkMergeWith = [&](SCCDAGPartition *part) -> void {
      if (!LDI->partitions.canMergePartitions(partition, part)) { errs() << "DSWP:   CANNOT MERGE\n"; return; }
      part->print(errs() << "DSWP:   CAN MERGE WITH PARTITION:\n", "DSWP:   ");

      auto demoMerged = LDI->partitions.demoMergePartitions(partition, part);
      if (demoMerged->cost > maxAllowedCost) return ;
      errs() << "DSWP:   Max allowed cost: " << maxAllowedCost << "\n";

      auto loweredCost = part->cost + partition->cost - demoMerged->cost;
      errs() << "DSWP:   Merging (cost " << partition->cost << ", " << part->cost << ") yields cost " << demoMerged->cost << "\n";
      if (loweredCost > maxLoweredCost) {
        errs() << "DSWP:   WILL MERGE IF BEST\n";
        minPartition = part;
        maxLoweredCost = loweredCost;
      }
    };

    /*
     * Check merge criteria on dependents and depth-1 neighbors
     */
    auto dependents = LDI->partitions.getDependents(partition);
    auto cousins = LDI->partitions.getCousins(partition);
    for (auto part : dependents) checkMergeWith(part);
    for (auto part : cousins) checkMergeWith(part);

    /*
     * Merge partition if one is found; reiterate the merge check on it
     */
    if (minPartition) {
      auto mergedPart = LDI->partitions.mergePartitions(partition, minPartition);
      partToCheck.push(mergedPart);
      mergedPart->print(errs() << "DSWP:   MERGED PART: " << partToCheck.size() << "\n", "DSWP:   ");
    }

    /*
     * Iterate the merge check on all dependent partitions
     */
    for (auto part : dependents) {
      if (minPartition == part) continue;
      partToCheck.push(part);
      part->print(errs() << "DSWP:   WILL CHECK: " << partToCheck.size() << "\n", "DSWP:   ");
    }
  }

  return ;
}
