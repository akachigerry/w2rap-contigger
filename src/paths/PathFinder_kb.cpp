//
// Created by Katie Barr (EI) on 31/10/2016.
//

//
// Created by Bernardo Clavijo (TGAC) on 11/07/2016.
//


#include "PathFinder_kb.h"
using namespace PatherKB;

void PathFinderkb::init_prev_next_vectors(){
    //TODO: this is stupid duplication of the digraph class, but it's so weird!!!
    prev_edges.resize(mToLeft.size());
    next_edges.resize(mToRight.size());
    for (auto e=0;e<mToLeft.size();++e){

        uint64_t prev_node=mToLeft[e];

        prev_edges[e].resize(mHBV.ToSize(prev_node));
        for (int i=0;i<mHBV.ToSize(prev_node);++i){
            prev_edges[e][i]=mHBV.EdgeObjectIndexByIndexTo(prev_node,i);
        }

        uint64_t next_node=mToRight[e];

        next_edges[e].resize(mHBV.FromSize(next_node));
        for (int i=0;i<mHBV.FromSize(next_node);++i){
            next_edges[e][i]=mHBV.EdgeObjectIndexByIndexFrom(next_node,i);
        }
    }


}

void PathFinderkb::mapEdgesToLMPReads(){
    std::cout << "edge mapping begun" << std::endl;
    ReadPathVec pRPV_lmp;
    vecKmerPath pKP_lmp;
    auto edges = mHBV.Edges();
    vecbvec edges_vec;
    BigDict<28> bigDict(300);
    BigKMerizer<28> tkmerizer(&bigDict);
    std::cout << "dict created" << std::endl;
    for (auto edge: edges) { tkmerizer.kmerize(edge); edges_vec.push_back(edge);};
    std::cout << "dict entries populated" << std::endl;
    std::vector<int> fwdXlat, revXlat;
    fwdXlat.resize(edges.size(), -1);
    revXlat.resize(edges.size(), -1);
    // if order of edges doesn't change i think this should work
    // i think previous/next vectors, should work, or next at least, not sure about previous
    // Pather( vecbvec const& reads, BigKDict const& dict, vecbvec const& edge, std::vector<int> const& fwdXlat, std::vector<int> const& revXlat, ReadPathVec* pReadPaths, vecKmerPath* pKmerPaths )
    int edge_count = 0; // if we incrememnt this each time an edge would have been added in the original making of the fwd/revXlats, these should be the same?
    for (uint64_t i=0;i<edges.size();++i){
        //auto fwEdgeId = pHBV->EdgeObjectCount();
        auto fwEdgeId = edge_count;
        bvec edge = edges[i];
        fwdXlat[i]=fwEdgeId;
        edge_count += 1;
        if ( edge.getCanonicalForm() == CanonicalForm::PALINDROME ) {
            revXlat[i] = fwEdgeId;
        }
        else {
            auto bwEdgeId = edge_count;
            revXlat[i] = bwEdgeId;
            edge_count += 1;
        }
        // as vectors are created with size edges.size, incrementing edge count will cause it to go over
        std::cout << "edge count:" << edge_count << std::endl;
        std::cout << "i: " << i << std::endl;
    }
    std::cout << "fwd/backward lists created" << std::endl;
    // fwd/bward xlat are how it determines which the next edges are
    Pather<28> pather( lmp_data, bigDict, edges_vec,
                    fwdXlat, revXlat,
                    &pRPV_lmp, &pKP_lmp );
    std::cout << "pather created" << std::endl;
    for (auto i=0ul;i<lmp_data.size();++i) pather(i);
    std::cout << "mapping complete" << std::endl;
    // basically copied from gonza but with edge object instead of read
    /*KMerHasher<200> hasher;
    std::cout << "pe edge object count" << mHBV.EdgeObjectCount() << std::endl;
    std::cout << "pe edge object count" << lmp_data.EdgeObjectCount() << std::endl;
    auto edge =  mHBV.EdgeObject(e);
    auto itr = edge.begin();
    auto end = edge.end() - 200;
    size_t hash = hasher.hash(itr);
    BigKMer<200> kmer(edge, hash);
    std::set< std::pair<unsigned long, unsigned long> > mapped_lmp_reads;
    int not_found = 0;
    int found = 0;
    while (++itr < end) {
        std::cout << "inside loop, kmer: " << kmer.getBV() << std::endl;
        hash = hasher.stepF(itr);
        std::cout << "called heasher step through successfully" << std::endl;
        kmer.successor(hash);
        std::cout << "got kmer successor" << kmer.getBV() << "reverse complemented" << kmer.isRC() <<  std::endl;
        // this is hwere it gets the bad access
        //auto fkmer = lmpDict.lookup((kmer.isRC())?(kmer.rc()) : (kmer));
        auto fkmer = lmpDict.lookup(kmer);
        if (fkmer == 0){
            not_found += 1;
            std::cout << "not found:" << not_found << std::endl;
        } else {
            //std::cout << "found kmer:" << fkmer->getBV() << std::endl;
            // roughly, want to repeat this for each, find where this is in the lmp hbv, and where the next/previous is (this will be pair, next/prev depends on odd/even)
            // the diciotnary is based on the graph, so you'd hope there's enough info in there not to need to go back to the graph
            // for next edge to be the right one, need to make sure hbv is from reads, not edges.
            std::cout << "found: " << found << std::endl;
            auto read_id = &fkmer->getBV() - &lmpReads[0];
            unsigned long pair_id;
            pair_id = read_id % 2 == 0 ? read_id + 1 : read_id - 1; // need to check about reverse complement stuff here
            mapped_lmp_reads.insert(std::make_pair(read_id, pair_id));
            int cont = 0;
                std::cout << "Contig: " << cont
                          << " Kmer: " << (fkmer->isRC() ? kmer.rc().getBV() : kmer.getBV())
                          << " Offset: " << fkmer->getOffset()
                          << " Is RC: " << fkmer->isRC()
                          << " Context: " << fkmer->getContext()
                          << " BV size: " << fkmer->getBV().size()
                          << " BV # : " << &fkmer->getBV() - &lmpReads[0]
                          << std::endl << std::endl
                          << " " << mHBV.EdgeObject(&fkmer->getBV() - &lmpReads[0])
                          << std::endl;

        }
    }*/
}

std::string PathFinderkb::path_str(std::vector<uint64_t> path) {
    std::string s="[";
    for (auto p:path){
        s+=std::to_string(p)+":"+std::to_string(mInv[p])+" ";//+" ("+std::to_string(mHBV.EdgeObject(p).size())+"bp "+std::to_string(paths_per_kbp(p))+"ppk)  ";
    }
    s+="]";
    return s;
}


void PathFinderkb::untangle_complex_in_out_choices(uint64_t large_frontier_size, bool verbose_separation) {
    //find a complex path
    uint64_t qsf=0,qsf_paths=0;
    uint64_t msf=0,msf_paths=0;
    init_prev_next_vectors();// need to clarify what mToLeft and mToRight are, guessing they're just into/out of each node
    std::cout<<"vectors initialised"<<std::endl;
    std::set<std::array<std::vector<uint64_t>,2>> seen_frontiers,solved_frontiers;
    std::vector<std::vector<uint64_t>> paths_to_separate;
    for (int e = 0; e < mHBV.EdgeObjectCount(); ++e) {
        if (e < mInv[e] && mHBV.EdgeObject(e).size() < large_frontier_size) {
            auto f=get_all_long_frontiers(e, large_frontier_size);// return edges in and out of this edge which represent long sequences of bases
            // think f[0] is vec of edge ids going into this edge, and f[1] is vec of edge ids going out
            if (f[0].size()>1 and f[1].size()>1 and seen_frontiers.count(f)==0){ // this determines whether it s acomplex edge
                // if theres more than 1 edge in, and more than 1 edge out, and we haven't already dealt with this in/out combination
                seen_frontiers.insert(f);
                bool single_dir=true;
                // not sure about this, seems to be determining whether same edge is both going into and out of current edge
                for (auto in_e:f[0]) for (auto out_e:f[1]) if (in_e==out_e) {single_dir=false;break;}
                if (single_dir) {
                    std::cout<<" Single direction frontiers for complex region on edge "<<e<<" IN:"<<path_str(f[0])<<" OUT: "<<path_str(f[1])<<std::endl;
                    std::vector<int> in_used(f[0].size(),0);// initialise vec for each in edge
                    std::vector<int> out_used(f[1].size(),0); //  ditto out edge
                    std::vector<std::vector<uint64_t>> first_full_paths;
                    bool reversed=false;
                    for (auto in_i=0;in_i<f[0].size();++in_i) {
                        auto in_e=f[0][in_i];
                        for (auto out_i=0;out_i<f[1].size();++out_i) {
                            auto out_e=f[1][out_i];
                            auto shared_paths = 0;
                            for (auto inp:mEdgeToPathIds[in_e]) // find paths associated with in_e
                                for (auto outp:mEdgeToPathIds[out_e]) // ditto out edge
                                    if (inp == outp) {// if they're on the same path

                                        shared_paths++;
                                        if (shared_paths==1){//not the best solution, but should work-ish
                                            std::vector<uint64_t > pv;
                                            for (auto e:mPaths[inp]) pv.push_back(e); // create vector of edges on theshared pat
                                            std::cout<<"found first path from "<<in_e<<" to "<< out_e << path_str(pv)<< std::endl;
                                            first_full_paths.push_back({});
                                            int16_t ei=0;
                                            while (mPaths[inp][ei]!=in_e) ei++; // find which edge on the path is the in edge
                                            // add all edges on the path until the out edge
                                            while (mPaths[inp][ei]!=out_e && ei<mPaths[inp].size()) first_full_paths.back().push_back(mPaths[inp][ei++]);
                                            if (ei>=mPaths[inp].size()) {
                                                std::cout<<"reversed path detected!"<<std::endl;
                                                reversed=true;
                                            }
                                            first_full_paths.back().push_back(out_e);
                                            //std::cout<<"added!"<<std::endl;
                                        }
                                    }
                            //check for reverse paths too- same but on inovlution
                            for (auto inp:mEdgeToPathIds[mInv[out_e]])
                                for (auto outp:mEdgeToPathIds[mInv[in_e]])
                                    if (inp == outp) {

                                        shared_paths++;
                                        if (shared_paths==1){//not the best solution, but should work-ish
                                            std::vector<uint64_t > pv;
                                            for (auto e=mPaths[inp].rbegin();e!=mPaths[inp].rend();++e) pv.push_back(mInv[*e]);
                                            std::cout<<"found first path from "<<in_e<<" to "<< out_e << path_str(pv)<< std::endl;
                                            first_full_paths.push_back({});
                                            int16_t ei=0;
                                            while (pv[ei]!=in_e) ei++;

                                            while (pv[ei]!=out_e && ei<pv.size()) first_full_paths.back().push_back(pv[ei++]);
                                            if (ei>=pv.size()) {
                                                std::cout<<"reversed path detected!"<<std::endl;
                                                reversed=true;
                                            }
                                            first_full_paths.back().push_back(out_e);
                                            //std::cout<<"added!"<<std::endl;
                                        }
                                    }
                            if (shared_paths) {
                                out_used[out_i]++;
                                in_used[in_i]++;
                                //std::cout << "  Shared paths " << in_e << " --> " << out_e << ": " << shared_paths << std::endl;

                            }
                        }
                    }
                    if ((not reversed) and std::count(in_used.begin(),in_used.end(),1) == in_used.size() and
                        std::count(out_used.begin(),out_used.end(),1) == out_used.size()){
                        std::cout<<" REGION COMPLETELY SOLVED BY PATHS!!!"<<std::endl;
                        solved_frontiers.insert(f);
                        for (auto p:first_full_paths) paths_to_separate.push_back(p);
                    }
                }

            }
        }
    }
    std::cout<<"Complex Regions solved by paths: "<<solved_frontiers.size() <<"/"<<seen_frontiers.size()<<" comprising "<<paths_to_separate.size()<<" paths to separate"<< std::endl;
    //std::cout<<"Complex Regions quasi-solved by paths (not acted on): "<< qsf <<"/"<<seen_frontiers.size()<<" comprising "<<qsf_paths<<" paths to separate"<< std::endl;
    //std::cout<<"Multiple Solution Regions (not acted on): "<< msf <<"/"<<seen_frontiers.size()<<" comprising "<<msf_paths<<" paths to separate"<< std::endl;

    uint64_t sep=0;
    std::map<uint64_t,std::vector<uint64_t>> old_edges_to_new;
    /*for (auto p:paths_to_separate){

        if (old_edges_to_new.count(p.front()) > 0 or old_edges_to_new.count(p.back()) > 0) {
            std::cout<<"WARNING: path starts or ends in an already modified edge, skipping"<<std::endl;
            continue;
        }

        auto oen=separate_path(p, verbose_separation);
        if (oen.size()>0) {
            for (auto et:oen){
                if (old_edges_to_new.count(et.first)==0) old_edges_to_new[et.first]={};
                for (auto ne:et.second) old_edges_to_new[et.first].push_back(ne);
            }
            sep++;
        }
    }
    if (old_edges_to_new.size()>0) {
        migrate_readpaths(old_edges_to_new);
    }
    std::cout<<" "<<sep<<" paths separated!"<<std::endl;*/
}



std::array<std::vector<uint64_t>,2> PathFinderkb::get_all_long_frontiers(uint64_t e, uint64_t large_frontier_size){
    //TODO: return all components in the region
    std::set<uint64_t> seen_edges, to_explore={e}, in_frontiers, out_frontiers;

    while (to_explore.size()>0){
        std::set<uint64_t> next_to_explore;
        for (auto x:to_explore){ //to_explore: replace rather and "update" (use next_to_explore)

            if (!seen_edges.count(x)){

                //What about reverse complements and paths that include loops that "reverse the flow"?
                if (seen_edges.count(mInv[x])) return std::array<std::vector<uint64_t>,2>(); //just cancel for now

                for (auto p:prev_edges[x]) {
                    if (mHBV.EdgeObject(p).size() >= large_frontier_size )  {
                        //What about frontiers on both sides?
                        in_frontiers.insert(p);
                        for (auto other_n:next_edges[p]){
                            if (!seen_edges.count(other_n)) {
                                if (mHBV.EdgeObject(other_n).size() >= large_frontier_size) {
                                    out_frontiers.insert(other_n);
                                    seen_edges.insert(other_n);
                                }
                                else next_to_explore.insert(other_n);
                            }
                        }
                    }
                    else if (!seen_edges.count(p)) next_to_explore.insert(p);
                }

                for (auto n:next_edges[x]) {
                    if (mHBV.EdgeObject(n).size() >= large_frontier_size) {
                        //What about frontiers on both sides?
                        out_frontiers.insert(n);
                        for (auto other_p:prev_edges[n]){
                            if (!seen_edges.count(other_p)) {
                                if (mHBV.EdgeObject(other_p).size() >= large_frontier_size) {
                                    in_frontiers.insert(other_p);
                                    seen_edges.insert(other_p);
                                }
                                else next_to_explore.insert(other_p);
                            }
                        }
                    }
                    else if (!seen_edges.count(n)) next_to_explore.insert(n);
                }
                seen_edges.insert(x);
            }
            if (seen_edges.size()>50) {
                return std::array<std::vector<uint64_t>,2>();
            }

        }
        to_explore=next_to_explore;
    }

    if (to_explore.size()>0) return std::array<std::vector<uint64_t>,2>();
    //the "canonical" representation is the one that has the smalled edge on the first vector, and bot ordered

    if (in_frontiers.size()>0 and out_frontiers.size()>0) {
        uint64_t min_in=*in_frontiers.begin();
        for (auto i:in_frontiers){
            if (i<min_in) min_in=i;
            if (mInv[i]<min_in) min_in=mInv[i];
        }
        uint64_t min_out=*out_frontiers.begin();
        for (auto i:out_frontiers){
            if (i<min_out) min_out=i;
            if (mInv[i]<min_out) min_out=mInv[i];
        }
        if (min_out<min_in){
            std::set<uint64_t> new_in, new_out;
            for (auto e:in_frontiers) new_out.insert(mInv[e]);
            for (auto e:out_frontiers) new_in.insert(mInv[e]);
            in_frontiers=new_in;
            out_frontiers=new_out;
        }
    }

    //std::sort(in_frontiers.begin(),in_frontiers.end());
    //std::sort(out_frontiers.begin(),out_frontiers.end());
    std::array<std::vector<uint64_t>,2> frontiers={std::vector<uint64_t>(in_frontiers.begin(),in_frontiers.end()),std::vector<uint64_t>(out_frontiers.begin(),out_frontiers.end())};



    return frontiers;
}