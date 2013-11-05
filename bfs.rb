def bfs(g, root)
  q = [root]
  while q != []
    r = q.shift
    q += (g[r] or [])
    yield r
  end
end

def dfs(g, root)
  q = [root]
  while q != []
    r = q.pop
    q += (g[r] or []).reverse
    yield r
  end
end

g = {
  'a' => ['b', 'c'],
  'b' => ['d', 'e'],
  'c' => ['f'],
  'f' => ['g'],
}

bfs(g, 'a') {|i| puts i }
puts '---'
dfs(g, 'a') {|i| puts i }
