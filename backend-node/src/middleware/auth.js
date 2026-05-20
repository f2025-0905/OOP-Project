const jwt = require('jsonwebtoken');

const SECRET = process.env.JWT_SECRET || 'shiftwise-dev-secret';

const generateToken = (userId, role) =>
  jwt.sign({ user_id: userId, role }, SECRET, { expiresIn: '24h' });

const verifyToken = (token) => {
  try { return jwt.verify(token, SECRET); }
  catch { return null; }
};

const requireAuth = (req, res, next) => {
  const header = req.headers.authorization;
  if (!header || !header.startsWith('Bearer ')) {
    return res.status(401).json({ error: 'Unauthorized. Please log in.' });
  }
  const decoded = verifyToken(header.slice(7));
  if (!decoded) return res.status(401).json({ error: 'Invalid or expired token.' });
  req.user = { userId: decoded.user_id, role: decoded.role };
  next();
};

const requireManager = (req, res, next) => {
  requireAuth(req, res, () => {
    if (req.user.role !== 'manager')
      return res.status(403).json({ error: 'Forbidden. Manager access required.' });
    next();
  });
};

module.exports = { generateToken, requireAuth, requireManager };
