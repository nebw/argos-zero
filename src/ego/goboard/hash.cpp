//
// Copyright 2006 and onwards, Lukasz Lew
//

#include "hash.hpp"

Hash::Hash() {}

uint Hash::Index() const { return hash; }

uint Hash::Lock() const { return hash >> 32; }

void Hash::Randomize(FastRandom& fr) {
    hash = (uint64(fr.GetNextUint()) << (0 * 16)) ^ (uint64(fr.GetNextUint()) << (1 * 16)) ^
           (uint64(fr.GetNextUint()) << (2 * 16)) ^ (uint64(fr.GetNextUint()) << (3 * 16));
}

void Hash::SetZero() { hash = 0; }

bool Hash::operator==(const Hash& other) const { return hash == other.hash; }

void Hash::operator^=(const Hash& other) { hash ^= other.hash; }

// -----------------------------------------------------------------------------

Zobrist::Zobrist() : hashes(Hash()) {
    FastRandom fr(123);
    ForEachNat(Player, pl) {
        ForEachNat(Vertex, v) {
            Move m = Move(pl, v);
            hashes[m].Randomize(fr);
        }
    }
    ForEachNat(Vertex, v) { koHashes[v].Randomize(fr); }
    ForEachNat(Player, p) { playerHashes[p].Randomize(fr); }
}

Hash Zobrist::OfMove(Move m) const { return hashes[m]; }

Hash Zobrist::OfPlayerVertex(Player pl, Vertex v) const { return hashes[Move(pl, v)]; }

Hash Zobrist::OfKoVertex(Vertex v) const { return koHashes[v]; }

Hash Zobrist::OfPlayer(Player p) const { return playerHashes[p]; }
