\name{bayesTest}
\alias{bayesTest}
\alias{testBayes}
\alias{simpleBayesTest}
\alias{multipleBayesTest}
\alias{print.multipleBayesTest}
\alias{plot.multipleBayesTest}
\title{
  Tests for sienaBayes results with print and plot methods
}
\description{
  These functions compute tests based on \code{\link{sienaBayesFit}} objects
  resulting from \code{\link{sienaBayes}}.
  Print and plot methods are available for the results of the
  multi-parameter test.
}
\usage{
simpleBayesTest(z, nfirst=z$nwarm+1, tested0=0,
                probs = c(0.025,0.975), ndigits=4)
multipleBayesTest(z, ..., nfirst=z$nwarm, tested0=0, ndigits=4)
\method{print}{multipleBayesTest}(x, ...)
\method{plot}{multipleBayesTest}(x, xlim=NULL, ylim=NULL, main=NULL, ...)
}
%- maybe also "usage" for other objects documented here.
\arguments{
  \item{z}{A \code{\link{sienaBayesFit}} object,
    resulting from a call to \code{\link{sienaBayes}.}}
  \item{nfirst}{The first element of the MCMC chain used for calculating
      properties of the chain; this can be the first element for which
      it is assumed that convergence has occurred.}
  \item{tested0}{The value to be tested; for \code{simpleBayesTest} this must be
     a number (applied to all coordinates),
     for \code{multipleBayesTest} it can be a number
     (applied to all coordinates) or a vector.}
  \item{probs}{A vector of two numbers between 0 and 1, the credibility limits
  for the credibility intervals.}
  \item{ndigits}{Number of digits to be given when representing the limits
   of the credibility intervals.}
  \item{\dots}{For \code{multipleBayesTest}: the numbers of the parameters
   to be tested (numbered as in the output of \code{simpleBayesTest});
   for \code{print.multipleBayesTest} and \code{plot.multipleBayesTest}:
   extra arguments.}
  \item{x}{A \code{\link{multipleBayesTest}} object,
    resulting from a call to \code{multipleBayesTest}.}
  \item{xlim}{Parameter \code{xlim} in \code{\link{plot}}; if \code{NULL},
   a sensible default will be used.}
  \item{ylim}{Parameter \code{ylim} in \code{\link{plot}}; if \code{NULL},
   a sensible default will be used.}
  \item{main}{Parameter \code{main} in \code{\link{plot}}; if \code{NULL},
    'posterior distances'.}
}
\details{
 For the default values of \code{probs} and \code{tested0},
 \code{simpleBayesTest} gives information also contained in
 \code{\link{print.sienaBayesFit}}.\cr
 To use \code{multipleBayesTest}, it is advisable first to execute
 \code{simpleBayesTest} to see the numbers that should be used in \dots.\cr
 \code{multipleBayesTest} calculates Mahalanobis distances of the elements
 of the posterior sample from the posterior mean, and the p-value is the
 relative frequency with which these are greater than the distance
 between the tested value and the posterior mean.
}
\value{\code{simpleBayesTest} produces a data frame containing for the non-rate
 effects their names,
 an indication of whether or not they are varying parameters,
 and test results.
 For each parameter separately, (for the varying parameters) the across-group mean
 or (for the non-varying parameters) the value is tested.
 The test results are the posterior credibility intervals for the
 probability limits defined by \code{probs},
 and the posterior probabilities of values larger than \code{tested0}.\cr
 \code{multipleBayesTest} produces an object of type \code{multipleBayesTest}
 which is a list with values:
 \item{prob}{posterior p-value}
 \item{chisquared}{test statistic}
  \item{postDistances}{sample from posterior }
  \item{nullValue}{input value of \code{tested0}}
  \item{effectNames}{names of effects tested as given in \dots.}
 \code{print.multipleBayesTest} prints the p-value with an explanation.\cr
 \code{plot.multipleBayesTest} plots the density function of \code{x$postDistances}.\cr
  }
\references{See the manual and \url{http://www.stats.ox.ac.uk/~snijders/siena/}
}
\author{Tom Snijders}
\seealso{\code{\link{sienaBayes}}, \code{\link{print.sienaBayesFit}}}
\examples{
  Group1 <- sienaDependent(array(c(N3401, HN3401), dim=c(45, 45, 2)))
  Group3 <- sienaDependent(array(c(N3403, HN3403), dim=c(37, 37, 2)))
  Group4 <- sienaDependent(array(c(N3404, HN3404), dim=c(33, 33, 2)))
  Group6 <- sienaDependent(array(c(N3406, HN3406), dim=c(36, 36, 2)))
  dataset.1 <- sienaDataCreate(Friends = Group1)
  dataset.3 <- sienaDataCreate(Friends = Group3)
  dataset.4 <- sienaDataCreate(Friends = Group4)
  dataset.6 <- sienaDataCreate(Friends = Group6)
  FourGroups <- sienaGroupCreate(
        list(dataset.1, dataset.3, dataset.4, dataset.6))
  FourEffects <- getEffects(FourGroups)
  FourEffects <- includeEffects(FourEffects, transTrip)
  FourEffects <- setEffect(FourEffects, density, random=TRUE)
  FourEffects <- setEffect(FourEffects, recip, random=TRUE)
  print(FourEffects, includeRandoms=TRUE)
  # Note this also shows the "randomEffects" column.
  FourAlgo <- sienaAlgorithmCreate(projname = "FourGroups", maxlike=TRUE)
\dontrun{
  bayes.model <- sienaBayes(FourAlgo, data = FourGroups,
        effects = FourEffects, nwarm=10, nmain=25, nrunMHBatches=10)
  bayes.model
  simpleBayesTest(bayes.model, probs = c(0.05,0.95))
}
}
% Add one or more standard keywords, see file "KEYWORDS" in the
% R documentation directory.
\keyword{ tests }