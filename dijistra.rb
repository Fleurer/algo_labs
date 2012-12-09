require 'set'

def adjmap(g)
  m = {}
  g.each do |e, d|
    src, dst = e
    m[src] ||= Set[]
    m[src] << dst
  end
  m
end

def shortest_path(g, src, dst)
  adjs = adjmap(g)
  path = { src => [nil, 0] }
  q = [src]
  while q != []
    node = q.shift
    _, dist = path[node]
    (adjs[node] or []).each do |n|
      q << n
      d = g[[node, n]]
      if not path[n] or path[n][1] > dist+d 
        path[n] = [node, dist+d]
      end
    end
  end
  path
end

g = {
  [:a, :b] => 1,
  [:a, :c] => 2,
  [:b, :d] => 3,
  [:b, :e] => 4,
  [:c, :e] => 4,
  [:d, :f] => 2,
  [:e, :f] => 1
}

p shortest_path g, :a, :f
