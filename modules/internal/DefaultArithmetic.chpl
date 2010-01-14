config param debugDefaultDist = false;

class DefaultDist: BaseDist {
  def dsiNewArithmeticDom(param rank: int, type idxType, param stridable: bool)
    return new DefaultArithmeticDom(rank, idxType, stridable, this);

  def dsiNewAssociativeDom(type idxType)
    return new DefaultAssociativeDom(idxType, this);

  def dsiNewOpaqueDom(type idxType)
    return new DefaultOpaqueDom(this);

  def dsiNewSparseDom(param rank: int, type idxType, dom: domain)
    return new DefaultSparseDom(rank, idxType, this, dom);

  def dsiClone() return this;

  //
  // should this be a Domain class method because (1) we may want to
  // take advantage of an existing domain class and (2) the new domain
  // that we are going to build may have to have a different
  // distribution (due to rank change, for example, but also maybe due
  // to slice).
  //
  def dsiBuildArithmeticDom(param rank: int, type idxType, param stridable: bool,
                            ranges: rank*range(idxType,
                                               BoundedRangeType.bounded,
                                               stridable)) {
    var dom = new DefaultArithmeticDom(rank, idxType, stridable, this);
    for i in 1..rank do
      dom.ranges(i) = ranges(i);
    return dom;
  }
}

//
// Note that the replicated copies are set up in ChapelLocale on the
// other locales.  This just sets it up on this locale.
//
pragma "private" var defaultDist = distributionValue(new DefaultDist());

class DefaultArithmeticDom: BaseArithmeticDom {
  param rank : int;
  type idxType;
  param stridable: bool;
  var dist: DefaultDist;
  var ranges : rank*range(idxType,BoundedRangeType.bounded,stridable);

  def linksDistribution() param return false;

  def DefaultArithmeticDom(param rank, type idxType, param stridable, dist) {
    this.dist = dist;
  }

  def dsiClear() {
    var emptyRange: range(idxType, BoundedRangeType.bounded, stridable);
    for param i in 1..rank do
      ranges(i) = emptyRange;
  }
  
  // dsi
  // function and iterator versions, also for setIndices
  def getIndices() return ranges;

  // dsi
  def setIndices(x) {
    if ranges.size != x.size then
      compilerError("rank mismatch in domain assignment");
    if ranges(1).eltType != x(1).eltType then
      compilerError("index type mismatch in domain assignment");
    ranges = x;
  }

  def these_help(param d: int) {
    if d == rank - 1 {
      for i in ranges(d) do
        for j in ranges(rank) do
          yield (i, j);
    } else {
      for i in ranges(d) do
        for j in these_help(d+1) do
          yield (i, (...j));
    }
  }

  def these_help(param d: int, block) {
    if d == block.size - 1 {
      for i in block(d) do
        for j in block(block.size) do
          yield (i, j);
    } else {
      for i in block(d) do
        for j in these_help(d+1, block) do
          yield (i, (...j));
    }
  }

  // dsiDefaultIterator
  def these() {
    if rank == 1 {
      for i in ranges(1) do
        yield i;
    } else {
      for i in these_help(1) do
        yield i;
    }
  }

  // dsiDefaultIterator
  def these(param tag: iterator) where tag == iterator.leader {
    if debugDefaultDist then
      writeln("*** In domain leader code: this = ", this);
    var numCores = here.numCores;
    var runningTasks = here.runningTasks();
    if debugDefaultDist then
      writeln("    numCores=", numCores, ", runningTasks=", runningTasks);

    var numChunks: uint(64) =
    if (runningTasks >= numCores) then 1
    else if maxChunks == -1 then
      if maxThreads == 0 then (numCores-runningTasks+1):uint(64)
      else (min(numCores-runningTasks+1, maxThreads)):uint(64)
    else if maxThreads == 0 then (min(numCores-runningTasks+1, maxChunks)):uint(64)
      else (min(numCores-runningTasks+1, min(maxThreads, maxChunks))):uint(64);

    var numelems: uint(64) = 1;
    for param i in 1..rank do
      numelems *= ranges(i).length:uint(64);

    if debugDefaultDist then
      writeln("*** DI: rank=", rank, " numelems=", numelems, " numChunks=", numChunks, " ranges(1).length=", ranges(1).length);

    if ((numelems <= minElemsPerChunk*numChunks) ||
        ((ranges(1).length):uint(64) < numChunks) ||
        (numChunks == 1)) {
      if debugDefaultDist then
	writeln("*** minElemsPerChunk*numChunks = ",
		minElemsPerChunk*numChunks, ", using 1 chunk");
      if rank == 1 {
	yield tuple(0..ranges(1).length-1);
      } else {
	var block: rank*range(idxType);
	for param i in 1..rank do
	  block(i) = 0..ranges(i).length-1;
	yield block;
      }
    } else {
      var locBlock: rank*range(idxType);
      for param i in 1..rank do
	locBlock(i) = 0:ranges(i).low.type..#(ranges(i).length);
      if debugDefaultDist then
	writeln("*** DI: locBlock = ", locBlock);
      coforall chunk in 0..numChunks-1 {
	var tuple: rank*range(idxType) = locBlock;
	const (lo,hi) = _computeMyChunk(locBlock(1).length, locBlock(1).high,
                                        numChunks, chunk);
	tuple(1) = lo..hi;
	if debugDefaultDist then
	  writeln("*** DI: tuple = ", tuple);
	yield tuple;
      }
    }
  }

  // dsiDefaultIterator
  def these(param tag: iterator, follower) where tag == iterator.follower {
    if debugDefaultDist then
      writeln("In domain follower code: Following ", follower);
    var block: ranges.type;
    if stridable {
      for param i in 1..rank do
        block(i) =
          if ranges(i).stride > 0 then
            ranges(i).low+follower(i).low*ranges(i).stride:ranges(i).eltType..ranges(i).low+follower(i).high*ranges(i).stride:ranges(i).eltType by ranges(i).stride
          else
            ranges(i).high+follower(i).high*ranges(i).stride:ranges(i).eltType..ranges(i).high+follower(i).low*ranges(i).stride:ranges(i).eltType by ranges(i).stride;
    } else {
      for  param i in 1..rank do
        block(i) = follower(i) + ranges(i).low;
    }
    if rank == 1 {
      for i in block do
        yield i;
    } else {
      for i in these_help(1, block) do
        yield i;
    }
  }

  // dsiMember (we don't need this version)
  //  version in wrapper record can call below version
  def member(ind: idxType) where rank == 1 {
    if !ranges(1).member(ind) then
      return false;
    return true;
  }

  // dsiMember
  def member(ind: rank*idxType) {
    for param i in 1..rank do
      if !ranges(i).member(ind(i)) then
        return false;
    return true;
  }

  // use below version by having wrapper record call below
  def order(ind: idxType) where rank == 1 {
    return ranges(1).order(ind);
  }

  // dsiIndexOrder, to the user: indexOrder
  def order(ind: rank*idxType) {
    var totOrder: idxType;
    var blk: idxType = 1;
    for param d in 1..rank by -1 {
      const orderD = ranges(d).order(ind(d));
      if (orderD == -1) then return orderD;
      totOrder += orderD * blk;
      blk *= ranges(d).length;
    }
    return totOrder;
  }

  // can we use order above to implement position by passing in an
  // optional argument that contains the dimensions.
  // use below version by having wrapper record call below
  def position(ind: idxType) where rank == 1 {
    var pos: 1*idxType;
    pos(1) = order(ind);
    return pos;
  }

  // dsiPosition
  def position(ind: rank*idxType) {
    var pos: rank*idxType;
    for d in 1..rank {
      pos(d) = ranges(d).order(ind(d));
    }
    return pos;
  }

  // dsiDims
  def dims()
    return ranges;

  // dsiDim
  def dim(d : int)
    return ranges(d);

  // dsiDim (optional), is this necesary? probably not now that
  // homogeneous tuples are implemented as C vectors.
  def dim(param d : int)
    return ranges(d);

  // dsiNumIndices
  def numIndices {
    var sum = 1:idxType;
    for param i in 1..rank do
      sum *= ranges(i).length;
    return sum;
    // WANT: return * reduce (this(1..rank).length);
  }

  // dsiLow
  def low {
    if rank == 1 {
      return ranges(1)._low;
    } else {
      var result: rank*idxType;
      for param i in 1..rank do
        result(i) = ranges(i)._low;
      return result;
    }
  }

  // dsiHigh
  def high {
    if rank == 1 {
      return ranges(1)._high;
    } else {
      var result: rank*idxType;
      for param i in 1..rank do
        result(i) = ranges(i)._high;
      return result;
    }
  }

  // dsiBuildArray
  def buildArray(type eltType) {
    return new DefaultArithmeticArr(eltType=eltType, rank=rank, idxType=idxType,
                                  /*stridable=stridable, reindexed=false, */
                                    stridable=stridable, reindexed=true,
                                    dom=this);
  }

  // dsiSlice
  // can we move this functionality to ChapelArray and use
  // dsiBuildArithmeticDom?
  def slice(param stridable: bool, ranges) {
    var d = new DefaultArithmeticDom(rank, idxType, stridable, dist);
    for param i in 1..rank do
      d.ranges(i) = dim(i)(ranges(i));
    return d;
  }

  // dsiRankChange, or can we move this functionality to ChapelArray as above?
  def rankChange(param rank: int, param stridable: bool, args) {
    def isRange(r: range(?)) param return true;
    def isRange(r) param return false;

    var d = new DefaultArithmeticDom(rank, idxType, stridable, dist);
    var i = 1;
    for param j in 1..args.size {
      if isRange(args(j)) {
        d.ranges(i) = dim(j)(args(j));
        i += 1;
      }
    }
    return d;
  }

  // dsiStrideBy, use 1 function, or can this be moved to ChapelArray as above.
  def strideBy(str : rank*int) {
    var x = new DefaultArithmeticDom(rank, idxType, true, dist);
    for i in 1..rank do
      x.ranges(i) = ranges(i) by str(i);
    return x;
  }

  def strideBy(str : int) {
    var x = new DefaultArithmeticDom(rank, idxType, true, dist);
    for i in 1..rank do
      x.ranges(i) = ranges(i) by str;
    return x;
  }
}

class DefaultArithmeticArr: BaseArr {
  type eltType;
  param rank : int;
  type idxType;
  param stridable: bool;
  //param reindexed: bool = false; // may have blk(rank) != 1
  param reindexed: bool = true; // may have blk(rank) != 1

  var dom : DefaultArithmeticDom(rank=rank, idxType=idxType,
                                         stridable=stridable);
  var off: rank*idxType;
  var blk: rank*idxType;
  var str: rank*int;
  var origin: idxType;
  var factoredOffs: idxType;
  var size : idxType;
  var data : _ddata(eltType);
  var noinit: bool = false;

  def canCopyFromDevice param return true;

  // end class definition here, then defined secondary methods below

  // dsi? can the compiler create this automatically?
  def getBaseDom() return dom;

  // dsiDestroyData
  def destroyData() {
    if dom.numIndices > 0 {
      pragma "no copy" pragma "no auto destroy" var dr = data;
      pragma "no copy" pragma "no auto destroy" var dv = __primitive("get ref", dr);
      pragma "no copy" pragma "no auto destroy" var er = __primitive("array_get", dv, 0);
      pragma "no copy" pragma "no auto destroy" var ev = __primitive("get ref", er);
      if (chpl__maybeAutoDestroyed(ev)) {
        for i in 0..dom.numIndices-1 {
          pragma "no copy" pragma "no auto destroy" var dr = data;
          pragma "no copy" pragma "no auto destroy" var dv = __primitive("get ref", dr);
          pragma "no copy" pragma "no auto destroy" var er = __primitive("array_get", dv, i);
          pragma "no copy" pragma "no auto destroy" var ev = __primitive("get ref", er);
          chpl__autoDestroy(ev);
        }
      }
    }
    delete data;
  }

  // dsiCreateAlias
  def makeAlias(B: DefaultArithmeticArr) {
    var A = B.reindex(dom);
    off = A.off;
    blk = A.blk;
    str = A.str;
    origin = A.origin;
    factoredOffs = A.factoredOffs;
    data = A.data;
    delete A;
  }

  // dsiDefaultIterator
  def these() var {
    if rank == 1 {
      var first = getDataIndex(dom.low);
      var second = getDataIndex(dom.low+dom.ranges(1).stride:idxType);
      var step = (second-first):int;
      var last = first + (dom.numIndices-1) * step:idxType;
      for i in first..last by step do
        yield data(i);
    } else {
      for i in dom do
        yield this(i);
    }
  }

  // dsiDefaultIterator
  def these(param tag: iterator) where tag == iterator.leader {
    if debugDefaultDist then
      writeln("*** In array leader code: [\n", this, "]");
    var numCores = here.numCores;
    var runningTasks = here.runningTasks();
    if debugDefaultDist then
      writeln("    numCores=", numCores, ", runningTasks=", runningTasks());
    var numChunks: uint(64) =
    if (runningTasks >= numCores) then 1
    else if maxChunks == -1 then
      if maxThreads == 0 then (numCores-runningTasks+1):uint(64)
      else (min(numCores-runningTasks+1, maxThreads)):uint(64)
    else if maxThreads == 0 then (min(numCores-runningTasks+1, maxChunks)):uint(64)
      else (min(numCores-runningTasks+1, min(maxThreads, maxChunks))):uint(64);

    var numelems: uint(64) = 1;
    for param i in 1..rank do
      numelems *= dom.ranges(i).length:uint(64);

    if debugDefaultDist then
      writeln("*** AI: rank=", rank, " numelems=", numelems, " numChunks=", numChunks, " dom.ranges(1).length=", dom.ranges(1).length);

    if ((numelems <= minElemsPerChunk*numChunks) ||
        ((dom.ranges(1).length):uint(64) < numChunks) ||
        (numChunks == 1)) {
      if debugDefaultDist then
	writeln("*** minElemsPerChunk*numChunks = ",
		minElemsPerChunk*numChunks, ", using 1 chunk");
      if rank == 1 {
	yield tuple(0..dom.ranges(1).length-1);
      } else {
	var block: rank*range(idxType);
	for param i in 1..rank do
	  block(i) = 0..dom.ranges(i).length-1;
	yield block;
      }
    } else {
      var locBlock: rank*range(idxType);
      for param i in 1..rank do
	locBlock(i) = 0:dom.ranges(i).low.type..#(dom.ranges(i).length);
      if debugDefaultDist then
	writeln("*** AI: locBlock = ", locBlock);
      coforall chunk in 0..numChunks-1 {
	var tuple: rank*range(idxType) = locBlock;
	const (lo,hi) = _computeMyChunk(locBlock(1).length, locBlock(1).high,
                                        numChunks, chunk);
	tuple(1) = lo..hi;
	if debugDefaultDist then
	  writeln("*** AI: tuple = ", tuple);
	yield tuple;
      }
    }
  }

  // dsiDefaultIterator
  def these(param tag: iterator, follower) var where tag == iterator.follower {
    if debugDefaultDist then
      writeln("*** In array follower code: [\n", this, "]");
    for i in dom.these(tag=iterator.follower, follower) do
      yield this(i);
  }

  def computeFactoredOffs() {
    factoredOffs = 0:idxType;
    for i in 1..rank do {
      factoredOffs = factoredOffs + blk(i) * off(i);
    }
  }
  
  // change name to setup and call after constructor call sites
  // we want to get rid of all initialize functions everywhere
  def initialize() {
    if noinit == true then return;
    for param dim in 1..rank {
      off(dim) = dom.dim(dim)._low;
      str(dim) = dom.dim(dim)._stride;
    }
    blk(rank) = 1:idxType;
    for param dim in 1..rank-1 by -1 do
      blk(dim) = blk(dim+1) * dom.dim(dim+1).length;
    computeFactoredOffs();
    size = blk(1) * dom.dim(1).length;
    data = new _ddata(eltType);
    data.init(size);
  }

  pragma "inline"
  def getDataIndex(ind: idxType ...1) where rank == 1
    return getDataIndex(ind);

  pragma "inline"
  def getDataIndex(ind: rank* idxType) {
    var sum = origin;
    if stridable {
      for param i in 1..rank do
        sum += (ind(i) - off(i)) * blk(i) / str(i):idxType;
    } else {
      if reindexed {
        for param i in 1..rank do
          sum += ind(i) * blk(i);
      } else {
        for param i in 1..rank-1 do
          sum += ind(i) * blk(i);
        sum += ind(rank);
      }
      sum -= factoredOffs;
    }
    return sum;
  }

  // dsiAccess
  // only need second version because wrapper record can pass a 1-tuple
  pragma "inline"
  def this(ind: idxType ...1) var where rank == 1
    return this(ind);
  pragma "inline"
  def this(ind : rank*idxType) var {
    if boundsChecking then
      if !dom.member(ind) then
        halt("array index out of bounds: ", ind);
    return data(getDataIndex(ind));
  }

  // dsiReindex
  def reindex(d: DefaultArithmeticDom) {
    if rank != d.rank then
      compilerError("illegal implicit rank change");
    for param i in 1..rank do
      if d.dim(i).length != dom.dim(i).length then
        halt("extent in dimension ", i, " does not match actual");
    var alias = new DefaultArithmeticArr(eltType=eltType, rank=d.rank,
                                         idxType=d.idxType,
                                         stridable=d.stridable,
                                         reindexed=true, dom=d, noinit=true);
    alias.data = data;
    alias.size = size: d.idxType;
    for param i in 1..rank {
      alias.off(i) = d.dim(i)._low;
      alias.blk(i) = (blk(i) * dom.dim(i)._stride / str(i)) : d.idxType;
      alias.str(i) = d.dim(i)._stride;
    }
    alias.origin = origin:d.idxType;
    alias.computeFactoredOffs();
    return alias;
  }

  // dsiCheckSlice
  // can this be put into ChapelArray.chpl and shared amongst all?
  def checkSlice(ranges) {
    for param i in 1..rank do
      if !dom.dim(i).boundsCheck(ranges(i)) then
        halt("array slice out of bounds in dimension ", i, ": ", ranges(i));
  }

  // dsiSlice
  def slice(d: DefaultArithmeticDom) {
    var alias = new DefaultArithmeticArr(eltType=eltType, rank=rank,
                                         idxType=idxType,
                                         stridable=d.stridable,
                                         reindexed=reindexed,
                                         dom=d, noinit=true);
    alias.data = data;
    alias.size = size;
    alias.blk = blk;
    alias.str = str;
    alias.origin = origin;
    for param i in 1..rank {
      alias.off(i) = d.dim(i)._low;
      alias.origin += blk(i) * (d.dim(i)._low - off(i)) / str(i);
    }
    alias.computeFactoredOffs();
    return alias;
  }

  // pull out into ChapelArray
  // define isRange as isCollapsedDimension (define in ChapelArray
  // too) and call this from within distribution classes in their
  // dsiRankChange implementations
  def checkRankChange(args) {
    def isRange(r: range(?e,?b,?s)) param return 1;
    def isRange(r) param return 0;

    for param i in 1..args.size do
      if isRange(args(i)) then
        if !dom.dim(i).boundsCheck(args(i)) then
          halt("array slice out of bounds in dimension ", i, ": ", args(i));
  }

  // dsiRankChange
  def rankChange(d, param newRank: int, param newStridable: bool, args) {
    def isRange(r: range(?e,?b,?s)) param return 1;
    def isRange(r) param return 0;

    var alias = new DefaultArithmeticArr(eltType=eltType, rank=newRank,
                                         idxType=idxType,
                                         stridable=newStridable, reindexed=true,
                                         dom=d, noinit=true);
    alias.data = data;
    alias.size = size;
    var i = 1;
    alias.origin = origin;
    for param j in 1..args.size {
      if isRange(args(j)) {
        alias.off(i) = d.dim(i)._low;
        alias.origin += blk(j) * (d.dim(i)._low - off(j)) / str(j);
        alias.blk(i) = blk(j);
        alias.str(i) = str(j);
        i += 1;
      } else {
        alias.origin += blk(j) * (args(j) - off(j)) / str(j);
      }
    }
    alias.computeFactoredOffs();
    return alias;
  }

  // dsiReallocate
  def reallocate(d: domain) {
    if (d._value.type == dom.type) {
      var copy = new DefaultArithmeticArr(eltType=eltType, rank=rank,
                                          idxType=idxType,
                                          stridable=d._value.stridable,
                                          reindexed=reindexed, dom=d._value);
      for i in d((...dom.ranges)) do
        copy(i) = this(i);
      off = copy.off;
      blk = copy.blk;
      str = copy.str;
      origin = copy.origin;
      factoredOffs = copy.factoredOffs;
      size = copy.size;
      destroyData();
      data = copy.data;
      delete copy;
    } else {
      halt("illegal reallocation");
    }
  }

  // move into ChapelArray and handle by calling accessor function on array
  def tupleInit(b: _tuple) {
    def _tupleInitHelp(j, param rank: int, b: _tuple) {
      if rank == 1 {
        for param i in 1..b.size {
          j(this.rank-rank+1) = dom.dim(this.rank-rank+1).low + i - 1;
          this(j) = b(i);
        }
      } else {
        for param i in 1..b.size {
          j(this.rank-rank+1) = dom.dim(this.rank-rank+1).low + i - 1;
          _tupleInitHelp(j, rank-1, b(i));
        }
      }
    }

    if rank == 1 {
      for param i in 1..b.size do
        this(this.dom.dim(1).low + i - 1) = b(i);
    } else {
      var j: rank*int;
      _tupleInitHelp(j, rank, b);
    }
  }
}

// dsiSerialWrite
def DefaultArithmeticDom.writeThis(f: Writer) {
  f.write("[", dim(1));
  for i in 2..rank do
    f.write(", ", dim(i));
  f.write("]");
}

// dsiSerialWrite
def DefaultArithmeticArr.writeThis(f: Writer) {
  if dom.numIndices == 0 then return;
  var i : rank*idxType;
  for dim in 1..rank do
    i(dim) = dom.dim(dim)._low;
  label next while true {
    f.write(this(i));
    if i(rank) <= (dom.dim(rank)._high - dom.dim(rank)._stride:idxType) {
      f.write(" ");
      i(rank) += dom.dim(rank)._stride:idxType;
    } else {
      for dim in 1..rank-1 by -1 {
        if i(dim) <= (dom.dim(dim)._high - dom.dim(dim)._stride:idxType) {
          i(dim) += dom.dim(dim)._stride:idxType;
          for dim2 in dim+1..rank {
            f.writeln();
            i(dim2) = dom.dim(dim2)._low;
          }
          continue next;
        }
      }
      break;
    }
  }
}

//
// this should be unified with versions in BlockDist and ChapelRange,
// use the one in BlockDist because it is correct, where should it go?
// ChapelRange or ChapelArray?  ChapelRange is more of a root.
//
//
// helper function for blocking index ranges
// - based on _computeBlock() but specialized for cases
//   where the low bound is always zero
//
def _computeMyChunk(numelems, wayhi, numblocks, blocknum) {
  if debugDefaultDist then
    writeln("in _computeMyChunk: numelems=",  numelems, " wayhi=", wayhi,
	    " numblocks=", numblocks, " blocknum=", blocknum);
  /*
  if numblocks == 1 then
    return (0:wayhi.type, numelems:wayhi.type-1);
  */

  if numelems == 0 then
    return (1:wayhi.type, 0:wayhi.type);

  def procToData(x)
    return x:wayhi.type + (x:real != x:int:real):wayhi.type;

  const blo =
    if blocknum == 0 then 0
      else procToData((numelems:real * blocknum) / numblocks);
  const bhi =
    if blocknum == numblocks - 1 then wayhi
      else procToData((numelems:real * (blocknum+1)) / numblocks) - 1;

  return (blo, bhi);
}
