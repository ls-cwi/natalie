/*
 * outputcompstatamc.h
 *
 *  Created on: 20-mar-2012
 *     Authors: M. El-Kebir, M.E. van der Wees
 */

#ifndef OUTPUTCOMPSTATAMC_H
#define OUTPUTCOMPSTATAMC_H

#include <ostream>
#include <fstream>
#include "output/output.h"
#include "score/scoremodel.h"

namespace nina {
namespace gna {

template<typename GR, typename BGR>
class OutputCompStatAmc : public Output<GR, BGR>
{
public:
  /// The graph type of the input graphs
  typedef GR Graph;
  /// The graph type of the bipartite matching graph
  typedef BGR BpGraph;
  /// Base class type
  typedef Output<Graph, BpGraph> Parent;

  using Parent::_matchingGraph;

private:
  TEMPLATE_GRAPH_TYPEDEFS(Graph);

  typedef typename BpGraph::Node BpNode;
  typedef typename BpGraph::Edge BpEdge;
  typedef typename BpGraph::NodeIt BpNodeIt;
  typedef typename BpGraph::EdgeIt BpEdgeIt;
  typedef typename BpGraph::IncEdgeIt BpIncEdgeIt;
  typedef typename BpGraph::RedNode BpRedNode;
  typedef typename BpGraph::BlueNode BpBlueNode;
  typedef typename BpGraph::RedNodeIt BpRedNodeIt;
  typedef typename BpGraph::BlueNodeIt BpBlueNodeIt;
  typedef typename Parent::MatchingGraphType MatchingGraphType;
  typedef typename Parent::BpMatchingMapType BpMatchingMapType;
  typedef typename Parent::OutputType OutputType;
  typedef ScoreModel<Graph, BpGraph> ScoreModelType;
  typedef typename std::set<BpBlueNode> BpBlueNodeSet;
  typedef typename BpBlueNodeSet::const_iterator BpBlueNodeSetIt;

  struct Entry {
    Node _node1;
    Node _node2;
    BpEdge _bpEdge;

    Entry(const Node node1,
          const Node node2,
          const BpEdge bpEdge)
      : _node1(node1)
      , _node2(node2)
      , _bpEdge(bpEdge)
    {
    }
  };

  typedef typename std::vector<Entry> EntryVector;
  typedef typename EntryVector::const_iterator EntryVectorIt;

  const ScoreModelType& _scoreModel;
  double _score;

public:
  OutputCompStatAmc(const MatchingGraphType& matchingGraph,
                    const ScoreModelType& scoreModel,
                    double score)
    : Parent(matchingGraph)
    , _scoreModel(scoreModel)
    , _score(score)
  {
  }

  void write(const BpMatchingMapType& matchingMap,
             OutputType outputType, std::ostream& outFile) const;

  std::string getExtension() const
  {
    return "-stat.csv";
  }
};

template<typename GR, typename BGR>
inline void OutputCompStatAmc<GR, BGR>::write(const BpMatchingMapType& matchingMap,
                                          OutputType outputType,
                                          std::ostream& out) const
{
  const BpGraph& gm = _matchingGraph.getGm();

  // we assume that G_1 is the query network and G_2 the target network
  EntryVector entries;
  BpBlueNodeSet blueNodeSet;

  // generate the query nodes first that have a match
  for (BpRedNodeIt r(gm); r != lemon::INVALID; ++r)
  {
    BpEdge e = matchingMap[r];
    if (e != lemon::INVALID)
    {
      BpBlueNode b = gm.blueNode(e);
      blueNodeSet.insert(b);

      Entry entry(_matchingGraph.mapGmToG1(r), _matchingGraph.mapGmToG2(b), e);
      entries.push_back(entry);
    }
  }

  double _bitComponent = 0;
  int _nAlignments = 0;
  int _nConserved = 0;
  int _nNonConservedInQuery = 0;
  int _nNonConservedInTarget = 0;
  for (EntryVectorIt it1 = entries.begin(); it1 != entries.end(); it1++)
  {
    if (it1->_node1 != lemon::INVALID && it1->_node2 != lemon::INVALID)
    {
      _nAlignments++;
      _bitComponent += _scoreModel.getWeightGm(it1->_bpEdge);
    }

    for (EntryVectorIt it2 = it1; it2 != entries.end(); it2++)
    {
      bool queryEdge = (it1->_node1 != lemon::INVALID && it2->_node1 != lemon::INVALID) ?
        _matchingGraph.getEdgeG1(it1->_node1, it2->_node1) != lemon::INVALID : false;

      bool targetEdge = (it1->_node2 != lemon::INVALID && it2->_node2 != lemon::INVALID) ?
        _matchingGraph.getEdgeG2(it1->_node2, it2->_node2) != lemon::INVALID : false;

      if (queryEdge && targetEdge)
        _nConserved++;

      if (targetEdge && !queryEdge)
        _nNonConservedInTarget++;

      if (queryEdge && !targetEdge)
        _nNonConservedInQuery++;
    }
  }

  double topComponent = _score - _bitComponent;
  if (fabs(topComponent) < 0.00001) topComponent = 0;

  out << "\"Number of aligned pairs\"" << ",\"" << _nAlignments << "\"" << std::endl
      << "\"Conserved interactions\"" << ",\"" << _nConserved << "\"" << std::endl
      << "\"Non-conserved interactions in human\"" << ",\"" << _nNonConservedInQuery << "\"" << std::endl
      << "\"Non-conserved interactions in mouse\"" << ",\"" << _nNonConservedInTarget << "\"" << std::endl
      << "\"Score\"" << ",\"" << _score << "\"" << std::endl;
      //<< "\"Sequence contribution\"" << ",\"" << _bitComponent << "\"" << std::endl
      //<< "\"Topology contribution\"" << ",\"" << topComponent << "\"" << std::endl;
}

} // namespace gna
} // namespace nina

#endif // OUTPUTCOMPSTATAMC_H_
