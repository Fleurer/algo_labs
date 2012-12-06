require 'algorithms'

# Each 'way' is assumpted as sorted. 

class NWayMerger
  def initialize(files)
    @heap = Containers::Heap.new {|a, b| (a[0] <=> b[0]) == -1 }
    files.each do |f|
      num = f.readline.to_i
      @heap.push([num, f])
    end
  end

  def pop
    r, f = @heap.pop
    n = f.readline.to_i
    @heap.push([n, f])
    r
  rescue
    r
  end
end

m = NWayMerger.new([
  open('/tmp/1.txt'),
  open('/tmp/2.txt'),
  open('/tmp/3.txt'),
])

while r = m.pop
  puts r
end
